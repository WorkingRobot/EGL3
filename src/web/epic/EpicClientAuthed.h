#pragma once

#include "../../utils/Callback.h"
#include "../BaseClient.h"
#include "../Http.h"
#include "../Response.h"
#include "responses/GetAccount.h"
#include "responses/GetAccountExternalAuths.h"
#include "responses/GetAccounts.h"
#include "responses/GetDeviceAuths.h"
#include "responses/GetExchangeCode.h"
#include "responses/GetDefaultBillingAccount.h"
#include "responses/GetAssets.h"
#include "responses/GetDownloadInfo.h"
#include "responses/GetCurrencies.h"
#include "responses/GetCatalogItems.h"
#include "responses/GetEntitlements.h"
#include "responses/GetFriendsSummary.h"
#include "responses/GetFriends.h"
#include "responses/GetFriendsRequested.h"
#include "responses/GetFriendsSuggested.h"
#include "responses/GetBlockedUsers.h"
#include "responses/GetAvailableSettingValues.h"
#include "responses/GetSettingsForAccounts.h"
#include "responses/GetLauncherDownloadInfo.h"
#include "responses/GetLightswitchStatus.h"
#include "responses/QueryProfile.h"
#include "responses/OAuthToken.h"

namespace EGL3::Web::Epic {
    class EpicClientAuthed : public BaseClient {
    public:
        EpicClientAuthed(const Responses::OAuthToken& AuthData, const cpr::Authentication& AuthClient);
        EpicClientAuthed(EpicClientAuthed&&);

        ~EpicClientAuthed();

        // Account service

        Response<Responses::GetAccount> GetAccount();

        Response<Responses::GetAccountExternalAuths> GetAccountExternalAuths();

        // NOTE: You can only request a maximum of 100 accounts at a time!
        // Fortnite uses 50 at a time, so it's recommended to do the same
        Response<Responses::GetAccounts> GetAccounts(const std::vector<std::string>& Accounts);

        Response<Responses::GetAccounts::Account> GetAccountById(const std::string& Id);

        Response<Responses::GetAccounts::Account> GetAccountByDisplayName(const std::string& DisplayName);

        Response<Responses::GetAccounts::Account> GetAccountByEmail(const std::string& Email);

        Response<Responses::GetDeviceAuths> GetDeviceAuths();

        // You can optionally add a X-Epic-Device-Info header with JSON {"type": "Google","model":"Pixel 3","os":"10"}
        Response<Responses::GetDeviceAuths::DeviceAuth> CreateDeviceAuth();

        Response<Responses::GetExchangeCode> GetExchangeCode();


        // Launcher service

        Response<Responses::GetDefaultBillingAccount> GetDefaultBillingAccount();

        // URL I'm unsure of: GET https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/accounts/{AccountId}/wallet?currencyCode=USD
        // It just returns 204, not sure what it does exactly

        // Default label is Production (if omitted from the request), but the launcher always calls with Live
        // All platforms are technically valid, but they'll return an empty list
        // Some that I know for sure that work are "Windows", "Mac", "IOS"
        Response<Responses::GetAssets> GetAssets(const std::string& Platform, const std::string& Label);

        // Version is in format like "11.0.1-14907503+++Portal+Release-Live-Windows"
        Response<Responses::GetLauncherDownloadInfo::BuildStatus> CheckLauncherVersion(const std::string& CurrentVersion);

        // Label used to be Live-DurrBurger up until 10.18.11, now it's Live-EternalKnight
        Response<Responses::GetLauncherDownloadInfo> GetLauncherDownloadInfo(const std::string& Platform, const std::string& Label, const std::optional<std::string>& ClientVersion = std::nullopt, const std::optional<std::string>& MachineId = std::nullopt);

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

        const Responses::OAuthToken& GetAuthData() const;

        void SetKillTokenOnDestruct(bool Value = true);

        Utils::Callback<void(const Responses::OAuthToken&)> OnRefresh;

    private:
        bool EnsureTokenValidity();

        template<typename ResponseType, int SuccessStatusCode, bool RequiresAccount, class CallFunctorType>
        Response<ResponseType> Call(CallFunctorType&& CallFunctor);

        std::mutex TokenValidityMutex;
        
        Responses::OAuthToken AuthData;
        std::string AuthHeader;
        cpr::Authentication AuthClient; // used for refresh token auth
        bool KillTokenOnDestruct;
    };
}
