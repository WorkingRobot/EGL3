#include "DeviceAuth.h"

namespace EGL3::Web::Epic::Auth {
    DeviceAuth::DeviceAuth(const cpr::Authentication& AuthClient, const std::string& AccountId, const std::string& DeviceId, const std::string& Secret) : AuthClient(AuthClient), AccountId(AccountId), DeviceId(DeviceId), Secret(Secret), Cancelled(false)
    {
        OAuthResponseFuture = std::async(std::launch::async, &DeviceAuth::RunOAuthResponseTask, this);
    }

    DeviceAuth::~DeviceAuth()
    {
        Cancelled = true;
        if (OAuthResponseFuture.valid()) {
            OAuthResponseFuture.wait();
        }
    }

    const std::shared_future<DeviceAuth::ErrorCode>& DeviceAuth::GetOAuthResponseFuture() const
    {
        return OAuthResponseFuture;
    }

    const rapidjson::Document& DeviceAuth::GetOAuthResponse() const
    {
        return OAuthResponse;
    }

    DeviceAuth::ErrorCode DeviceAuth::RunOAuthResponseTask()
    {
        auto Response = Http::Post(
            Http::FormatUrl<Host::Account>("oauth/token"),
            AuthClient,
            cpr::Payload{ { "grant_type", "device_auth" }, { "account_id", AccountId }, { "device_id", DeviceId }, { "secret", Secret } }
        );

        if (Cancelled) { return CANCELLED; }

        if (Response.status_code != 200) {
            return EXCH_CODE_NOT_200;
        }

        auto RespJson = Http::ParseJson(Response);

        if (RespJson.HasParseError()) {
            return EXCH_CODE_JSON;
        }

        OAuthResponse = std::move(RespJson);
        return SUCCESS;
    }
}
