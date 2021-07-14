#include "EpicClientAuthed.h"

#include "../../utils/Log.h"
#include "../Http.h"
#include "../RunningFunctionGuard.h"
#include "responses/ErrorInfo.h"
#include "EpicClient.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Epic {
    EpicClientAuthed::EpicClientAuthed(const Responses::OAuthToken& AuthData, const cpr::Authentication& AuthClient) :
        AuthData(AuthData),
        AuthHeader(std::format("{} {}", AuthData.TokenType, AuthData.AccessToken)),
        AuthClient(AuthClient),
        KillTokenOnDestruct(true)
    {

    }

    EpicClientAuthed::EpicClientAuthed(EpicClientAuthed&& Other) :
        AuthData(Other.AuthData),
        AuthHeader(Other.AuthHeader),
        AuthClient(Other.AuthClient)
    {
        Other.EnsureCallCompletion();
        std::lock_guard Lock(Other.TokenValidityMutex);
        Other.AuthHeader.clear();
        Other.AuthData = {};
    }

    EpicClientAuthed::~EpicClientAuthed()
    {
        EnsureCallCompletion();

        //                         Token isn't expired yet
        if (KillTokenOnDestruct && AuthData.ExpiresAt > TimePoint::clock::now()) {
            // This will return a 204
            // If it fails, it's on epic's blame. We don't really need any handling
            Http::Delete(
                Http::FormatUrl<Host::Account>("oauth/sessions/kill/{}", AuthData.AccessToken),
                cpr::Header{ { "Authorization", AuthHeader } }
            );
        }
    }

    Response<Responses::GetAccount> EpicClientAuthed::GetAccount()
    {
        return Call<Responses::GetAccount, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/{}", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetAccountExternalAuths> EpicClientAuthed::GetAccountExternalAuths()
    {
        return Call<Responses::GetAccountExternalAuths, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/{}/externalAuths", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }
    
    Response<Responses::GetAccounts> EpicClientAuthed::GetAccounts(const std::vector<std::string>& Accounts)
    {
        return Call<Responses::GetAccounts, 200, false>(
            [&, this]() {
                cpr::Parameters Parameters;
                for (auto& Item : Accounts) {
                    Parameters.Add({ "accountId", Item });
                }

                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account"),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    Parameters
                );
            }
        );
    }

    Response<Responses::GetAccounts::Account> EpicClientAuthed::GetAccountById(const std::string& Id)
    {
        return Call<Responses::GetAccounts::Account, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/{}", Id),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetAccounts::Account> EpicClientAuthed::GetAccountByDisplayName(const std::string& DisplayName)
    {
        return Call<Responses::GetAccounts::Account, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/displayName/{}", DisplayName),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetAccounts::Account> EpicClientAuthed::GetAccountByEmail(const std::string& Email)
    {
        return Call<Responses::GetAccounts::Account, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/email/{}", Email),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetDeviceAuths> EpicClientAuthed::GetDeviceAuths()
    {
        return Call<Responses::GetDeviceAuths, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/{}/deviceAuth", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetDeviceAuths::DeviceAuth> EpicClientAuthed::CreateDeviceAuth()
    {
        return Call<Responses::GetDeviceAuths::DeviceAuth, 200, true>(
            [this]() {
                return Http::Post(
                    Http::FormatUrl<Host::Account>("public/account/{}/deviceAuth", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
                    cpr::Body{ "{}" }
                );
            }
        );
    }

    Response<Responses::GetExchangeCode> EpicClientAuthed::GetExchangeCode()
    {
        return Call<Responses::GetExchangeCode, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("oauth/exchange", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetDefaultBillingAccount> EpicClientAuthed::GetDefaultBillingAccount()
    {
        return Call<Responses::GetDefaultBillingAccount, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/{}/billingaccounts/default", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetAssets> EpicClientAuthed::GetAssets(const std::string& Platform, const std::string& Label)
    {
        return Call<Responses::GetAssets, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Launcher>("public/assets/{}", Platform),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "label", Label } }
                );
            }
        );
    }

    Response<Responses::GetLauncherDownloadInfo::BuildStatus> EpicClientAuthed::CheckLauncherVersion(const std::string& CurrentVersion)
    {
        return Call<Responses::GetLauncherDownloadInfo::BuildStatus, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Launcher>("public/assets/info/launcher/{}", CurrentVersion),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetLauncherDownloadInfo> EpicClientAuthed::GetLauncherDownloadInfo(const std::string& Platform, const std::string& Label, const std::optional<std::string>& ClientVersion, const std::optional<std::string>& MachineId)
    {
        return Call<Responses::GetLauncherDownloadInfo, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Launcher>("public/assets/v2/platform/{}/launcher", Platform),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
                    cpr::Parameters{ { "label", Label }, { "clientVersion", ClientVersion.value_or("") }, { "machineId", MachineId.value_or("") } }
                );
            }
        );
    }

    Response<Responses::GetDownloadInfo> EpicClientAuthed::GetDownloadInfo(const std::string& Platform, const std::string& Label, const std::string& CatalogItemId, const std::string& AppName)
    {
        return Call<Responses::GetDownloadInfo, 200, false>(
            [&, this]() {
                return Http::Post(
                    Http::FormatUrl<Host::Launcher>("public/assets/v2/platform/{}/catalogItem/{}/app/{}/label/{}", Platform, CatalogItemId, AppName, Label),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
                    cpr::Body{ "{}" }
                );
            }
        );
    }

    Response<Responses::GetCurrencies> EpicClientAuthed::GetCurrencies(int Start, int Count)
    {
        return Call<Responses::GetCurrencies, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Catalog>("shared/currencies"),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
                );
            }
        );
    }

    Response<Responses::GetCatalogItems> EpicClientAuthed::GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails, bool IncludeMainGameDetails)
    {
        return Call<Responses::GetCatalogItems, 200, false>(
            [&, this]() {
                cpr::Parameters Parameters{
                    { "includeDLCDetails", std::to_string(IncludeDLCDetails) },
                    { "includeMainGameDetails", std::to_string(IncludeMainGameDetails) },
                    { "country", Country },
                    { "locale", Locale }
                };
                for (auto& Item : Items) {
                    Parameters.Add({ "id", Item });
                }

                return Http::Get(
                    Http::FormatUrl<Host::Catalog>("shared/namespace/{}/bulk/items", Namespace),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    Parameters
                );
            }
        );
    }

    Response<Responses::GetEntitlements> EpicClientAuthed::GetEntitlements(int Start, int Count)
    {
        return Call<Responses::GetEntitlements, 200, true>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Entitlement>("account/{}/entitlements", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
                );
            }
        );
    }

    Response<Responses::GetFriendsSummary> EpicClientAuthed::GetFriendsSummary()
    {
        return Call<Responses::GetFriendsSummary, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Friends>("v1/{}/summary", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "displayNames", "true" } }
                );
            }
        );
    }

    Response<Responses::GetFriends> EpicClientAuthed::GetFriends()
    {
        return Call<Responses::GetFriends, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "displayNames", "true" } }
                );
            }
        );
    }

    Response<Responses::GetFriendsRequested> EpicClientAuthed::GetFriendsInboundRequests()
    {
        return Call<Responses::GetFriendsRequested, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Friends>("v1/{}/incoming", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "displayNames", "true" } }
                );
            }
        );
    }

    Response<Responses::GetFriendsRequested> EpicClientAuthed::GetFriendsOutboundRequests()
    {
        return Call<Responses::GetFriendsRequested, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Friends>("v1/{}/outgoing", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "displayNames", "true" } }
                );
            }
        );
    }

    Response<Responses::GetFriendsSuggested> EpicClientAuthed::GetFriendsSuggested()
    {
        return Call<Responses::GetFriendsSuggested, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Friends>("v1/{}/suggested", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "displayNames", "true" } }
                );
            }
        );
    }

    Response<Responses::GetBlockedUsers> EpicClientAuthed::GetBlockedUsers()
    {
        return Call<Responses::GetBlockedUsers, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Friends>("v1/{}/blocklist", *AuthData.AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "displayNames", "true" } }
                );
            }
        );
    }

    Response<Responses::GetFriendsSummary::RealFriend> EpicClientAuthed::GetFriend(const std::string& AccountId)
    {
        return Call<Responses::GetFriendsSummary::RealFriend, 200, true>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends/{}", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "displayNames", "true" } }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::AddFriend(const std::string& AccountId)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Post(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends/{}", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Body{ "" }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::RemoveFriend(const std::string& AccountId)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Delete(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends/{}", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Body{ "" }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::SetFriendAlias(const std::string& AccountId, const std::string& Nickname)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Put(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends/{}/alias", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "text/plain" } },
                    cpr::Body{ Nickname }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::ClearFriendAlias(const std::string& AccountId)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Delete(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends/{}/alias", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Body{ "" }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::SetFriendNote(const std::string& AccountId, const std::string& Note)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Put(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends/{}/note", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "text/plain" } },
                    cpr::Body{ Note }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::ClearFriendNote(const std::string& AccountId)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Delete(
                    Http::FormatUrl<Host::Friends>("v1/{}/friends/{}/note", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Body{ "" }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::BlockUser(const std::string& AccountId)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Post(
                    Http::FormatUrl<Host::Friends>("v1/{}/blocklist/{}", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Body{ "" }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::UnblockUser(const std::string& AccountId)
    {
        return Call<void, 204, true>(
            [&, this]() {
                return Http::Delete(
                    Http::FormatUrl<Host::Friends>("v1/{}/blocklist/{}", *AuthData.AccountId, AccountId),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Body{ "" }
                );
            }
        );
    }

    Response<Responses::GetAvailableSettingValues> EpicClientAuthed::GetAvailableSettingValues(const std::string& Setting)
    {
        return Call<Responses::GetAvailableSettingValues, 200, true>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Channels>("v1/user/{}/setting/{}/available", *AuthData.AccountId, Setting),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetSettingsForAccounts> EpicClientAuthed::GetSettingsForAccounts(const std::vector<std::string>& Accounts, const std::initializer_list<std::string>& Settings)
    {
        return Call<Responses::GetSettingsForAccounts, 200, false>(
            [&, this]() {
                cpr::Parameters Parameters;

                for (auto& Account : Accounts) {
                    Parameters.Add({ "accountId", Account });
                }

                for (auto& Setting : Settings) {
                    Parameters.Add({ "settingKey", Setting });
                }

                return Http::Get(
                    Http::FormatUrl<Host::Channels>("v1/user/setting"),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    Parameters
                );
            }
        );
    }

    Response<void> EpicClientAuthed::UpdateAccountSetting(const std::string& Setting, const std::string& Value)
    {
        return Call<void, 204, true>(
            [&, this]() {
                rapidjson::StringBuffer Buf;
                {
                    rapidjson::Writer Writer(Buf);

                    Writer.StartObject();

                    Writer.Key("value");
                    Writer.String(Value.c_str(), Value.size());

                    Writer.EndObject();
                }

                return Http::Put(
                    Http::FormatUrl<Host::Channels>("v1/user/{}/setting/{}", *AuthData.AccountId, Setting),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
                    cpr::Body{ Buf.GetString(), Buf.GetSize() }
                );
            }
        );
    }

    Response<Responses::GetLightswitchStatus::ServiceStatus> EpicClientAuthed::GetLightswitchStatus(const std::string& AppName)
    {
        return Call<Responses::GetLightswitchStatus::ServiceStatus, 200, false>(
            [&, this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Lightswitch>("service/{}/status", AppName),
                    cpr::Header{ { "Authorization", AuthHeader } }
                );
            }
        );
    }

    Response<Responses::GetLightswitchStatus> EpicClientAuthed::GetLightswitchStatus(const std::initializer_list<std::string>& AppNames)
    {
        return Call<Responses::GetLightswitchStatus, 200, false>(
            [&, this]() {
                cpr::Parameters Parameters;
                for (auto& AppName : AppNames) {
                    Parameters.Add({ "serviceId", AppName });
                }
                return Http::Get(
                    Http::FormatUrl<Host::Lightswitch>("service/bulk/status"),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    Parameters
                );
            }
        );
    }

    Response<void> EpicClientAuthed::AuthorizeEOSClient(const std::string& ClientId, const std::initializer_list<std::string>& Scopes, const Responses::GetCsrfToken& CsrfToken)
    {
        return Call<void, 200, true>(
            [&, this]() {
                rapidjson::StringBuffer JsonBuffer;
                {
                    rapidjson::Writer<rapidjson::StringBuffer> Writer(JsonBuffer);

                    Writer.StartObject();

                    Writer.Key("scope");
                    Writer.StartArray();
                    for (auto& Scope : Scopes) {
                        Writer.String(Scope.c_str(), Scope.size());
                    }
                    Writer.EndArray();

                    Writer.EndObject();
                }

                return Http::Post(
                    Http::FormatUrl<Host::BaseEpicGames>("client/{}/authorize", ClientId),
                    cpr::Header{ { "X-XSRF-Token", CsrfToken.XsrfToken }, { "Content-Type", "application/json" } },
                    cpr::Cookies{ { "EPIC_BEARER_TOKEN", AuthData.AccessToken }, { "EPIC_SESSION_AP", CsrfToken.EpicSessionAp } },
                    cpr::Body{ JsonBuffer.GetString(), JsonBuffer.GetSize() }
                );
            }
        );
    }

    Response<void> EpicClientAuthed::UpdatePresenceStatus(const std::string& Namespace, const std::string& ConnectionId, const std::string& Status)
    {
        return Call<void, 200, false>(
            [&, this]() {
                rapidjson::StringBuffer JsonBuffer;
                {
                    rapidjson::Writer<rapidjson::StringBuffer> Writer(JsonBuffer);

                    Writer.StartObject();

                    Writer.Key("operationName");
                    Writer.String("updateStatus");

                    Writer.Key("variables");
                    Writer.StartObject();
                        Writer.Key("namespace");
                        Writer.String(Namespace.c_str(), Namespace.size());
                        Writer.Key("connectionId");
                        Writer.String(ConnectionId.c_str(), ConnectionId.size());
                        Writer.Key("status");
                        Writer.String(Status.c_str(), Status.size());
                    Writer.EndObject();

                    Writer.Key("query");
                    Writer.String("mutation updateStatus($namespace: String!, $connectionId: String!, $status: String!) {\n  PresenceV2 {\n    updateStatus(namespace: $namespace, connectionId: $connectionId, status: $status) {\n      success\n      __typename\n    }\n    __typename\n  }\n}\n");

                    Writer.EndObject();
                }

                return Http::Post(
                    Http::FormatUrl<Host::LauncherGql>(),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json" } },
                    cpr::Body{ JsonBuffer.GetString(), JsonBuffer.GetSize() },
                    cpr::UserAgent{ "EpicGamesLauncher" }
                );
            }
        );
    }

    const Responses::OAuthToken& EpicClientAuthed::GetAuthData() const {
        return AuthData;
    }

    void EpicClientAuthed::SetKillTokenOnDestruct(bool Value)
    {
        KillTokenOnDestruct = Value;
    }

    bool EpicClientAuthed::EnsureTokenValidity()
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
                Http::FormatUrl<Host::Account>("oauth/token"),
                AuthClient,
                cpr::Payload{ { "grant_type", "refresh_token" }, { "refresh_token", AuthData.RefreshToken.value() } }
            );
        }
        // Refresh tokens were not given in the oauth response, use client_credentials here
        else {
            Response = Http::Post(
                Http::FormatUrl<Host::Account>("oauth/token"),
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
    Response<ResponseType> EpicClientAuthed::Call(CallFunctorType&& CallFunctor) {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if constexpr (RequiresAccount) {
            if (!AuthData.AccountId.has_value()) { return ErrorData::Status::InvalidToken; }
        }

        if (!EnsureTokenValidity()) { return ErrorData::Status::InvalidToken; }

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        cpr::Response Response = std::invoke(std::move(CallFunctor));

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
