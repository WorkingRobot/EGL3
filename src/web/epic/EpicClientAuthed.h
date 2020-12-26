#pragma once

#include "../BaseClient.h"
#include "responses/Responses.h"

namespace EGL3::Web::Epic {
    class EpicClientAuthed : public BaseClient {
    public:
        EpicClientAuthed(const rapidjson::Document& OAuthResponse, const cpr::Authentication& AuthClient);

        ~EpicClientAuthed();

        // Account service

        Response<Responses::GetAccount> GetAccount();

        Response<Responses::GetAccountExternalAuths> GetAccountExternalAuths();

        // NOTE: You can only request a maximum of 100 accounts at a time!
        // Fortnite uses 50 at a time, so it's recommended to do the same
        Response<Responses::GetAccounts> GetAccounts(const std::vector<std::string>& Accounts);

        Response<Responses::GetDeviceAuths> GetDeviceAuths();

        // You can optionally add a X-Epic-Device-Info header with JSON {"type": "Google","model":"Pixel 3","os":"10"}
        Response<Responses::GetDeviceAuths::DeviceAuth> CreateDeviceAuth();


        // Launcher service

        Response<Responses::GetDefaultBillingAccount> GetDefaultBillingAccount();

        // URL I'm unsure of: GET https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/accounts/{AccountId}/wallet?currencyCode=USD
        // It just returns 204, not sure what it does exactly

        // Default label is Production (if omitted from the request), but the launcher always calls with Live
        // All platforms are technically valid, but they'll return an empty list
        // Some that I know for sure that work are "Windows", "Mac", "IOS"
        Response<Responses::GetAssets> GetAssets(const std::string& Platform, const std::string& Label);

        Response<Responses::GetDownloadInfo> GetDownloadInfo(const std::string& Platform, const std::string& Label, const std::string& CatalogItemId, const std::string& AppName);


        // Catalog service

        // You can actually get by without auth (e.g. open in browser) if you set start to 0 and count to 100
        // By ommitting start/count, it only returns 10 elements
        Response<Responses::GetCurrencies> GetCurrencies(int Start = 0, int Count = 100);

        // I have never seen those 2 booleans actually add something to the response
        Response<Responses::GetCatalogItems> GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails = false, bool IncludeMainGameDetails = false);


        // Entitlement service

        Response<Responses::GetEntitlements> GetEntitlements(int Start = 0, int Count = 5000);


        // Friends service

        Response<Responses::GetFriendsSummary> GetFriendsSummary();

        Response<Responses::GetFriends> GetFriends();

        Response<Responses::GetFriendsRequested> GetFriendsInboundRequests();

        Response<Responses::GetFriendsRequested> GetFriendsOutboundRequests();

        Response<Responses::GetFriendsSuggested> GetFriendsSuggested();

        Response<Responses::GetBlockedUsers> GetBlockedUsers();

        Response<Responses::GetFriendsSummary::RealFriend> GetFriend(const std::string& AccountId);

        Response<void> AddFriend(const std::string& AccountId);

        Response<void> RemoveFriend(const std::string& AccountId);

        Response<void> SetFriendAlias(const std::string& AccountId, const std::string& Nickname);

        Response<void> ClearFriendAlias(const std::string& AccountId);

        Response<void> SetFriendNote(const std::string& AccountId, const std::string& Note);

        Response<void> ClearFriendNote(const std::string& AccountId);

        Response<void> BlockUser(const std::string& AccountId);

        Response<void> UnblockUser(const std::string& AccountId);


        // Channels service

        // Valid setting values: "avatar", "avatarBackground", "appInstalled"
        Response<Responses::GetAvailableSettingValues> GetAvailableSettingValues(const std::string& Setting);

        Response<Responses::GetSettingsForAccounts> GetSettingsForAccounts(const std::vector<std::string>& Accounts, const std::initializer_list<std::string>& Settings);

        Response<void> UpdateAccountSetting(const std::string& Setting, const std::string& Value);


        // Lightswitch

        Response<Responses::GetLightswitchStatus::ServiceStatus> GetLightswitchStatus(const std::string& AppName);

        Response<Responses::GetLightswitchStatus> GetLightswitchStatus(const std::initializer_list<std::string>& AppNames);

        // MCP Requests

        // ProfileIds:
        // athena = br
        // collections = s14 fish collections
        // common_core = purchases and banners(?)
        // creative = islands and stuff
        // common_public = no idea, hasn't been updated in 2 years
        // metadata = stw storm shield stuff
        // campaign = stw
        // collection_book_schematics0 = stw collection book
        // collection_book_people0 = stw collection book
        // theater0 = stw world inventory
        // outpost0 = stw storm shield storage
        Response<Responses::QueryProfile> QueryProfile(const std::string& ProfileId, int Revision = -1);

    public:
        bool EnsureTokenValidity();

        std::mutex TokenValidityMutex;
        
        Responses::OAuthToken AuthData;
        std::string AuthHeader;
        cpr::Authentication AuthClient; // used for refresh token auth
    };
}
