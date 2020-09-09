#pragma once

#include "../http.h"
#include "responses/responses.h"

class EpicClient {
public:
    enum ErrorCode {
        ERROR_SUCCESS,
        ERROR_CANCELLED,
        ERROR_INVALID_TOKEN,
        ERROR_CODE_NOT_200,
        ERROR_CODE_NOT_JSON,
        ERROR_CODE_BAD_JSON
    };

    template<class T>
    class Response {
    public:
        bool HasError() const {
            return Error != ERROR_SUCCESS;
        }

        ErrorCode GetErrorCode() const {
            return Error;
        }

        T* operator->() const {
            return Data.get();
        }

        Response(ErrorCode Error) : Error(Error), Data(nullptr) {}
        Response(T&& Data) : Error(ERROR_SUCCESS), Data(std::make_unique<T>(std::forward<T&&>(Data))) {}

    private:
        ErrorCode Error;
        std::unique_ptr<T> Data;
    };

    EpicClient(const rapidjson::Document& OAuthResponse, const cpr::Authentication& AuthClient);

    ~EpicClient();

    // Account service

    Response<RespGetAccount> GetAccount();

    Response<RespGetAccountExternalAuths> GetAccountExternalAuths();

    // Launcher service

    Response<RespGetDefaultBillingAccount> GetDefaultBillingAccount();

    // URL I'm unsure of: GET https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/accounts/{AccountId}/wallet?currencyCode=USD
    // It just returns 204, not sure what it does

    // Default label is Production (if omitted from the request), but the launcher always calls with Live
    // All platforms are technically valid, but they'll return an empty list
    // Some that I know for sure that work are "Windows", "Mac", "IOS"
    Response<RespGetAssets> GetAssets(const std::string& Platform, const std::string& Label);

    // Catalog service

    // You can actually get by without auth (e.g. open in browser) if you set start to 0 and count to 100
    // By ommitting start/count, it only returns 10 elements
    Response<RespGetCurrencies> GetCurrencies(int Start = 0, int Count = 100);

    // I have never seen those 2 booleans actually add something to the response
    Response<RespGetCatalogItems> GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails = false, bool IncludeMainGameDetails = false);

    // Entitlement service

    Response<RespGetEntitlements> GetEntitlements(int Start = 0, int Count = 5000);

    // Friends service

    // I've only observed "steam" and "default"
    Response<RespGetExternalSourceSettings> GetExternalSourceSettings(const std::string& Platform);

    Response<RespGetFriends> GetFriends(bool IncludePending);

    Response<RespGetBlockedUsers> GetBlockedUsers();

private:
    class RunningFunctionGuard {
    public:
        RunningFunctionGuard(EpicClient& Client) : Client(Client) {
            {
                std::lock_guard<std::mutex> lock(Client.RunningFunctionMutex);
                Client.RunningFunctionCount++;
            }
            Client.RunningFunctionCV.notify_all();
        }

        ~RunningFunctionGuard() {
            {
                std::lock_guard<std::mutex> lock(Client.RunningFunctionMutex);
                Client.RunningFunctionCount--;
            }
            Client.RunningFunctionCV.notify_all();
        }

    private:
        EpicClient& Client;
    };

    bool EnsureTokenValidity();

    std::mutex TokenValidityMutex;

    std::mutex RunningFunctionMutex;
    std::condition_variable RunningFunctionCV;
    int32_t RunningFunctionCount;

    RespOAuthToken AuthData;
    std::string AuthHeader;
    cpr::Authentication AuthClient; // used for refresh token auth

    std::atomic_bool Cancelled;
};
