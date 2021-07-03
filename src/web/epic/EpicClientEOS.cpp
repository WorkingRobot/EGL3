#include "EpicClientEOS.h"

#include "../../utils/Log.h"
#include "../Http.h"
#include "../RunningFunctionGuard.h"
#include "responses/ErrorInfo.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Epic {
    EpicClientEOS::EpicClientEOS(const Responses::OAuthToken& AuthData, const cpr::Authentication& AuthClient, const std::string& DeploymentId) :
        AuthData(AuthData),
        AuthHeader(std::format("{} {}", AuthData.TokenType, AuthData.AccessToken)),
        AuthClient(AuthClient),
        DeploymentId(DeploymentId),
        KillTokenOnDestruct(true)
    {

    }

    EpicClientEOS::EpicClientEOS(EpicClientEOS&& Other) :
        AuthData(Other.AuthData),
        AuthHeader(Other.AuthHeader),
        AuthClient(Other.AuthClient)
    {
        Other.EnsureCallCompletion();
        std::lock_guard Lock(Other.TokenValidityMutex);
        Other.AuthHeader.clear();
        Other.AuthData = {};
    }

    EpicClientEOS::~EpicClientEOS()
    {
        EnsureCallCompletion();

        //                         Token isn't expired yet
        if (KillTokenOnDestruct && AuthData.ExpiresAt > TimePoint::clock::now()) {
            // This will return a 204
            // If it fails, it's on epic's blame. We don't really need any handling
            Http::Post(
                Http::FormatUrl<Host::EOS>("oauth/v1/revoke"),
                cpr::Header{ { "Authorization", AuthHeader } },
                cpr::Parameters{ { "token", AuthData.AccessToken } }
            );
        }
    }

    Response<void> EpicClientEOS::SetPresence(const std::string& ConnectionId, const Friends::UserPresence& Presence)
    {
        return Call<void, 200, true>(
            [&, this]() {
                rapidjson::StringBuffer StatusBuffer;
                {
                    rapidjson::Writer<rapidjson::StringBuffer> Writer(StatusBuffer);

                    Writer.StartObject();

                    Writer.Key("status");
                    Writer.String(Presence.Status.c_str(), Presence.Status.size());

                    Writer.Key("activity");
                        Writer.StartObject();
                        Writer.Key("value");
                        Writer.String(Presence.Activity.c_str(), Presence.Activity.size());
                        Writer.EndObject();

                    Writer.Key("props");
                        Writer.StartObject();
                        for (auto& [Name, Value] : Presence.Properties) {
                            Writer.Key(Name.c_str(), Name.size());
                            Writer.String(Value.c_str(), Value.size());
                        }
                        Writer.EndObject();

                    Writer.Key("conn");
                        Writer.StartObject();
                        Writer.Key("props");
                            Writer.StartObject();
                            for (auto& [Name, Value] : Presence.ConnectionProperties) {
                                Writer.Key(Name.c_str(), Name.size());
                                Writer.String(Value.c_str(), Value.size());
                            }
                            Writer.EndObject();
                        Writer.EndObject();

                    Writer.EndObject();
                }

                return Http::Patch(
                    Http::FormatUrl<Host::EOS>("presence/v1/{}/{}/presence/{}", DeploymentId, *AuthData.AccountId, ConnectionId),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
                    cpr::Body{ StatusBuffer.GetString(), StatusBuffer.GetSize() }
                );
            }
        );
    }

    const Responses::OAuthToken& EpicClientEOS::GetAuthData() const {
        return AuthData;
    }

    void EpicClientEOS::SetKillTokenOnDestruct(bool Value)
    {
        KillTokenOnDestruct = Value;
    }

    bool EpicClientEOS::EnsureTokenValidity()
    {
        // Make sure that no other thread will call in to this function and end up with the refresh token being used multiple times
        std::unique_lock Lock(TokenValidityMutex);

        auto Now = TimePoint::clock::now();

        if (AuthData.ExpiresAt > Now) {
            return true; // access token isn't invalid yet
        }

        cpr::Response Response;
        // Refresh tokens were given in the oauth response
        if (AuthData.RefreshToken.has_value()) {
            if (AuthData.RefreshExpiresAt.value() < Now) {
                return false; // refresh token is also invalid, what do we do here?
            }

            Response = Http::Post(
                Http::FormatUrl<Host::EOS>("oauth/v1/token"),
                AuthClient,
                cpr::Payload{ { "grant_type", "refresh_token" }, { "refresh_token", AuthData.RefreshToken.value() } }
            );
        }
        // Refresh tokens were not given in the oauth response, use client_credentials here
        else {
            Response = Http::Post(
                Http::FormatUrl<Host::EOS>("oauth/v1/token"),
                AuthClient,
                cpr::Payload{ { "grant_type", "client_credentials" } }
            );
        }

        if (Response.status_code != 200) {
            return false;
        }

        auto RespJson = Http::ParseJson(Response);

        if (RespJson.HasParseError()) {
            return false;
        }


        if (!EGL3_ENSURE(Responses::OAuthToken::Parse(RespJson, AuthData), LogLevel::Error, "OAuth data failed to parse")) {
            return false;
        }

        AuthHeader = std::format("{} {}", AuthData.TokenType, AuthData.AccessToken);
        Lock.unlock();
        OnRefresh(AuthData);
        return true;
    }

    template<typename ResponseType, int SuccessStatusCode, bool RequiresAccount, class CallFunctorType>
    Response<ResponseType> EpicClientEOS::Call(CallFunctorType&& CallFunctor) {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if constexpr (RequiresAccount) {
            if (!AuthData.AccountId.has_value()) { return ErrorData::Status::InvalidToken; }
        }

        if (!EnsureTokenValidity()) { return ErrorData::Status::InvalidToken; }

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        auto Response = std::invoke(std::move(CallFunctor));

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != SuccessStatusCode) {
            auto RespErrorJson = Http::ParseJson(Response);

            if (RespErrorJson.HasParseError()) {
                EGL3_LOG(LogLevel::Error, "Could not parse error message data (not json)");
                return Response.status_code;
            }

            Responses::ErrorInfo RespError;
            if (!Responses::ErrorInfo::Parse(RespErrorJson, RespError)) {
                EGL3_LOG(LogLevel::Error, "Could not parse error message data (bad json)");
                return Response.status_code;
            }
            return { Response.status_code, RespError.ErrorCode };
        }

        if constexpr (std::is_void_v<ResponseType>) {
            return ErrorData::Status::Success;
        }
        else {
            auto RespJson = Http::ParseJson(Response);

            if (RespJson.HasParseError()) {
                return ErrorData::Status::NotJson;
            }

            ResponseType Resp;
            if (!ResponseType::Parse(RespJson, Resp)) {
                return ErrorData::Status::BadJson;
            }

            return Resp;
        }
    }
}
