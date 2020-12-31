#include "EpicClientAuthed.h"

#include "../../utils/Assert.h"
#include "../Http.h"
#include "../RunningFunctionGuard.h"
#include "responses/ErrorInfo.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Epic {
    EpicClientAuthed::EpicClientAuthed(const rapidjson::Document& OAuthResponse, const cpr::Authentication& AuthClient) : AuthClient(AuthClient)
    {
        EGL3_CONDITIONAL_LOG(Responses::OAuthToken::Parse(OAuthResponse, AuthData), LogLevel::Critical, "OAuth data failed to parse");

        AuthHeader = AuthData.TokenType + " " + AuthData.AccessToken;
    }

    EpicClientAuthed::~EpicClientAuthed()
    {
        EnsureCallCompletion();

        // Token isn't expired yet
        if (AuthData.ExpiresAt > TimePoint::clock::now()) {
            // This will return a 204
            // If it fails, it's on epic's blame. We don't really need any handling
            Http::Delete(
                Http::FormatUrl<Host::Account>("oauth/sessions/kill/%s", AuthData.AccessToken.c_str()),
                cpr::Header{ { "Authorization", AuthHeader } }
            );
        }
    }

    Response<Responses::GetAccount> EpicClientAuthed::GetAccount()
    {
        return Call<Responses::GetAccount, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/%s", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Account>("public/account/%s/externalAuths", AuthData.AccountId->c_str()),
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
                    Parameters.AddParameter({ "accountId", Item }, cpr::CurlHolder());
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
                    Http::FormatUrl<Host::Account>("public/account/%s", Id.c_str()),
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
                    Http::FormatUrl<Host::Account>("public/account/displayName/%s", DisplayName.c_str()),
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
                    Http::FormatUrl<Host::Account>("public/account/email/%s", Email.c_str()),
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
                    Http::FormatUrl<Host::Account>("public/account/%s/deviceAuth", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Account>("public/account/%s/deviceAuth", AuthData.AccountId->c_str()),
                    cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
                    cpr::Body{ "{}" }
                );
            }
        );
    }

    Response<Responses::GetDefaultBillingAccount> EpicClientAuthed::GetDefaultBillingAccount()
    {
        return Call<Responses::GetDefaultBillingAccount, 200, true>(
            [this]() {
                return Http::Get(
                    Http::FormatUrl<Host::Account>("public/account/%s/billingaccounts/default", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Launcher>("public/assets/%s", Platform.c_str()),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    cpr::Parameters{ { "label", Label } }
                );
            }
        );
    }

    Response<Responses::GetDownloadInfo> EpicClientAuthed::GetDownloadInfo(const std::string& Platform, const std::string& Label, const std::string& CatalogItemId, const std::string& AppName)
    {
        return Call<Responses::GetDownloadInfo, 200, false>(
            [&, this]() {
                return Http::Post(
                    Http::FormatUrl<Host::Launcher>("public/assets/v2/platform/%s/catalogItem/%s/app/%s/label/%s", Platform.c_str(), CatalogItemId.c_str(), AppName.c_str(), Label.c_str()),
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
                    Parameters.AddParameter({ "id", Item }, cpr::CurlHolder());
                }

                return Http::Get(
                    Http::FormatUrl<Host::Catalog>("shared/namespace/%s/bulk/items", Namespace.c_str()),
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
                    Http::FormatUrl<Host::Entitlement>("account/%s/entitlements", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/summary", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/incoming", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/outgoing", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/suggested", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/blocklist", AuthData.AccountId->c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends/%s/alias", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends/%s/alias", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends/%s/note", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/friends/%s/note", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/blocklist/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Friends>("v1/%s/blocklist/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
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
                    Http::FormatUrl<Host::Channels>("v1/user/%s/setting/%s/available", AuthData.AccountId->c_str(), Setting.c_str()),
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
                    Parameters.AddParameter({ "accountId", Account }, cpr::CurlHolder());
                }

                for (auto& Setting : Settings) {
                    Parameters.AddParameter({ "settingKey", Setting }, cpr::CurlHolder());
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
                    Http::FormatUrl<Host::Channels>("v1/user/%s/setting/%s", AuthData.AccountId->c_str(), Setting.c_str()),
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
                    Http::FormatUrl<Host::Lightswitch>("service/%s/status", AppName.c_str()),
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
                    Parameters.AddParameter({ "serviceId", AppName }, cpr::CurlHolder());
                }
                return Http::Get(
                    Http::FormatUrl<Host::Lightswitch>("service/bulk/status"),
                    cpr::Header{ { "Authorization", AuthHeader } },
                    Parameters
                );
            }
        );
    }

    const Responses::OAuthToken& EpicClientAuthed::GetAuthData() const {
        return AuthData;
    }

    bool EpicClientAuthed::EnsureTokenValidity()
    {
        // Make sure that no other thread will call in to this function and end up with the refresh token being used multiple times
        std::lock_guard Lock(TokenValidityMutex);

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


        if (!EGL3_CONDITIONAL_LOG(Responses::OAuthToken::Parse(RespJson, AuthData), LogLevel::Error, "OAuth data failed to parse")) {
            return false;
        }

        AuthHeader = AuthData.TokenType + " " + AuthData.AccessToken;
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
