#include "AuthorizationCode.h"

namespace EGL3::Web::Epic::Auth {
    AuthorizationCode::AuthorizationCode(const cpr::Authentication& AuthClient, const std::string& Code) :
        AuthClient(AuthClient),
        Code(Code),
        Cancelled(false)
    {
        OAuthResponseFuture = std::async(std::launch::async, &AuthorizationCode::RunOAuthResponseTask, this);
    }

    AuthorizationCode::~AuthorizationCode()
    {
        Cancelled = true;
        if (OAuthResponseFuture.valid()) {
            OAuthResponseFuture.wait();
        }
    }

    const std::shared_future<AuthorizationCode::ErrorCode>& AuthorizationCode::GetOAuthResponseFuture() const
    {
        return OAuthResponseFuture;
    }

    const rapidjson::Document& AuthorizationCode::GetOAuthResponse() const
    {
        return OAuthResponse;
    }

    AuthorizationCode::ErrorCode AuthorizationCode::RunOAuthResponseTask()
    {
        auto Response = Http::Post(
            Http::FormatUrl<Host::Account>("oauth/token"),
            AuthClient,
            cpr::Payload{ { "grant_type", "authorization_code" }, { "token_type", "eg1" }, { "code", Code } }
        );

        if (Cancelled) { return CANCELLED; }

        if (Response.status_code != 200) {
            return AUTH_CODE_NOT_200;
        }

        auto RespJson = Http::ParseJson(Response);

        if (RespJson.HasParseError()) {
            return AUTH_CODE_JSON;
        }

        OAuthResponse = std::move(RespJson);
        return SUCCESS;
    }
}
