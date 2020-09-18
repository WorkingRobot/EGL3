#pragma once

#include "../BaseClient.h"
#include "responses/Responses.h"

namespace EGL3::Web::Epic {
    class EpicClientAuthed : public BaseClient {
    public:
        EpicClientAuthed(const rapidjson::Document& OAuthResponse, const cpr::Authentication& AuthClient);

        // Account service

        Response<Responses::GetAccount> GetAccount();

        Response<Responses::GetAccountExternalAuths> GetAccountExternalAuths();

        // Launcher service

        Response<Responses::GetDefaultBillingAccount> GetDefaultBillingAccount();

        // URL I'm unsure of: GET https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/accounts/{AccountId}/wallet?currencyCode=USD
        // It just returns 204, not sure what it does

        // Default label is Production (if omitted from the request), but the launcher always calls with Live
        // All platforms are technically valid, but they'll return an empty list
        // Some that I know for sure that work are "Windows", "Mac", "IOS"
        Response<Responses::GetAssets> GetAssets(const std::string& Platform, const std::string& Label);

        // Catalog service

        // You can actually get by without auth (e.g. open in browser) if you set start to 0 and count to 100
        // By ommitting start/count, it only returns 10 elements
        Response<Responses::GetCurrencies> GetCurrencies(int Start = 0, int Count = 100);

        // I have never seen those 2 booleans actually add something to the response
        Response<Responses::GetCatalogItems> GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails = false, bool IncludeMainGameDetails = false);

        // Entitlement service

        Response<Responses::GetEntitlements> GetEntitlements(int Start = 0, int Count = 5000);

        // Friends service

        // I've only observed "steam" and "default"
        Response<Responses::GetExternalSourceSettings> GetExternalSourceSettings(const std::string& Platform);

        Response<Responses::GetFriends> GetFriends(bool IncludePending);

        Response<Responses::GetBlockedUsers> GetBlockedUsers();

        // Lightswitch

        Response<Responses::GetLightswitchStatus::ServiceStatus> GetLightswitchStatus(const std::string& AppName);

        Response<Responses::GetLightswitchStatus> GetLightswitchStatus(const std::initializer_list<std::string>& AppNames);

    protected:
        void KillAuthentication() override;

    private:
        bool EnsureTokenValidity();

        std::mutex TokenValidityMutex;
        
        Responses::OAuthToken AuthData;
        std::string AuthHeader;
        cpr::Authentication AuthClient; // used for refresh token auth
    };
}
