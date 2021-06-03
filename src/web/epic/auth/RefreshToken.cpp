#include "RefreshToken.h"

namespace EGL3::Web::Epic::Auth {
    RefreshToken::RefreshToken(const cpr::Authentication& AuthClient, const std::string& Code) :
        AuthClient(AuthClient),
        Code(Code),
        Cancelled(false)
    {
        OAuthResponseFuture = std::async(std::launch::async, &RefreshToken::RunOAuthResponseTask, this);
    }

    RefreshToken::~RefreshToken()
    {
        Cancelled = true;
        if (OAuthResponseFuture.valid()) {
            OAuthResponseFuture.wait();
        }
    }

    const std::shared_future<RefreshToken::ErrorCode>& RefreshToken::GetOAuthResponseFuture() const
    {
        return OAuthResponseFuture;
    }

    const rapidjson::Document& RefreshToken::GetOAuthResponse() const
    {
        return OAuthResponse;
    }

    RefreshToken::ErrorCode RefreshToken::RunOAuthResponseTask()
    {
        auto Response = Http::Post(
            Http::FormatUrl<Host::Account>("oauth/token"),
            AuthClient,
            cpr::Payload{ { "grant_type", "refresh_token" }, { "token_type", "eg1" }, { "refresh_token", Code } }
        );

        if (Cancelled) { return CANCELLED; }

        if (Response.status_code != 200) {
            return REFRESH_CODE_NOT_200;
        }

        auto RespJson = Http::ParseJson(Response);

        if (RespJson.HasParseError()) {
            return REFRESH_CODE_JSON;
        }

        OAuthResponse = std::move(RespJson);
        return SUCCESS;
    }
}
