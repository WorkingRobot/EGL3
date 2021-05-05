#include "DeviceCode.h"

#include "../../ClientSecrets.h"

namespace EGL3::Web::Epic::Auth {
    DeviceCode::DeviceCode(const cpr::Authentication& AuthClient) : AuthClient(AuthClient), Cancelled(false)
    {
        BrowserUrlFuture = std::async(std::launch::async, &DeviceCode::RunBrowserUrlTask, this);
    }

    DeviceCode::~DeviceCode()
    {
        Cancelled = true;
        if (BrowserUrlFuture.valid()) {
            BrowserUrlFuture.wait();
        }
        if (OAuthResponseFuture.valid()) {
            OAuthResponseFuture.wait();
        }
    }

    const std::shared_future<DeviceCode::ErrorCode>& DeviceCode::GetBrowserUrlFuture() const
    {
        return BrowserUrlFuture;
    }

    const std::string& DeviceCode::GetBrowserUrl() const
    {
        return BrowserUrl;
    }

    const std::shared_future<DeviceCode::ErrorCode>& DeviceCode::GetOAuthResponseFuture() const
    {
        return OAuthResponseFuture;
    }

    const rapidjson::Document& DeviceCode::GetOAuthResponse() const
    {
        return OAuthResponse;
    }

    DeviceCode::ErrorCode DeviceCode::RunBrowserUrlTask()
    {
        auto ClientCredResponse = Http::Post(
            Http::FormatUrl<Host::Account>("oauth/token"),
            AuthClientDauntless,
            cpr::Payload{ { "grant_type", "client_credentials" } }
        );

        if (Cancelled) { return CANCELLED; }

        if (ClientCredResponse.status_code != 200) {
            return CLIENT_CREDS_NOT_200;
        }

        auto ClientCredJson = Http::ParseJson(ClientCredResponse);

        if (ClientCredJson.HasParseError()) {
            return CLIENT_CREDS_JSON;
        }

        // We don't care about any of the other parameters since this is only used once anyway
        auto& ClientCredAccessTokenValue = ClientCredJson["access_token"];
        auto ClientCredAccessToken = std::string(ClientCredAccessTokenValue.GetString(), ClientCredAccessTokenValue.GetStringLength());

        auto DeviceAuthResponse = Http::Post(
            Http::FormatUrl<Host::Account>("oauth/deviceAuthorization"),
            cpr::Header{ {"Authorization", "bearer " + ClientCredAccessToken } },
            cpr::Body{ }
        );

        if (Cancelled) { return CANCELLED; }

        if (DeviceAuthResponse.status_code != 200) {
            return DEVICE_AUTH_NOT_200;
        }

        auto DeviceAuthJson = Http::ParseJson(DeviceAuthResponse);

        if (DeviceAuthJson.HasParseError()) {
            return DEVICE_AUTH_JSON;
        }

        auto& DeviceDeviceCodeValue = DeviceAuthJson["device_code"];
        Code = std::string(DeviceDeviceCodeValue.GetString(), DeviceDeviceCodeValue.GetStringLength());

        ExpiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(DeviceAuthJson["expires_in"].GetInt64());

        // Halved because I don't feel like waiting 10 seconds OkayChamp
        RefreshInterval = std::chrono::seconds(DeviceAuthJson["interval"].GetInt64() / 2);

        auto& DeviceAuthBrowserUrlValue = DeviceAuthJson["verification_uri_complete"];
        BrowserUrl = std::string(DeviceAuthBrowserUrlValue.GetString(), DeviceAuthBrowserUrlValue.GetStringLength());

        OAuthResponseFuture = std::async(std::launch::async, &DeviceCode::RunOAuthResponseTask, this);

        return SUCCESS;
    }

    DeviceCode::ErrorCode DeviceCode::RunOAuthResponseTask()
    {
        if (Cancelled) { return CANCELLED; }

        auto NextSleep = std::chrono::steady_clock::now() + RefreshInterval;
        while (!Cancelled) {
            std::this_thread::sleep_until(NextSleep);
            if (Cancelled) { return CANCELLED; }
            if (std::chrono::steady_clock::now() > ExpiresAt) {
                return EXPIRED;
            }
            NextSleep += RefreshInterval;

            auto Response = Http::Post(
                Http::FormatUrl<Host::Account>("oauth/token"),
                AuthClient,
                cpr::Payload{ { "grant_type", "device_code" }, { "device_code", Code } }
            );
            if (Cancelled) { return CANCELLED; }

            if (Response.status_code != 200) {
                // User hasn't finished yet
                continue;
            }

            auto RespJson = Http::ParseJson(Response);

            if (RespJson.HasParseError()) {
                return DEVICE_CODE_JSON;
            }

            OAuthResponse = std::move(RespJson);
            return SUCCESS;
        }

        return CANCELLED;
    }
}
