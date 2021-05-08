#include "TokenToToken.h"

namespace EGL3::Web::Epic::Auth {
    TokenToToken::TokenToToken(const cpr::Authentication& AuthClient, const std::string& Token) : AuthClient(AuthClient), Token(Token), Cancelled(false)
    {
        OAuthResponseFuture = std::async(std::launch::async, &TokenToToken::RunOAuthResponseTask, this);
    }

    TokenToToken::~TokenToToken()
    {
        Cancelled = true;
        if (OAuthResponseFuture.valid()) {
            OAuthResponseFuture.wait();
        }
    }

    const std::shared_future<TokenToToken::ErrorCode>& TokenToToken::GetOAuthResponseFuture() const
    {
        return OAuthResponseFuture;
    }

    const rapidjson::Document& TokenToToken::GetOAuthResponse() const
    {
        return OAuthResponse;
    }

    TokenToToken::ErrorCode TokenToToken::RunOAuthResponseTask()
    {
        auto Response = Http::Post(
            Http::FormatUrl<Host::Account>("oauth/token"),
            AuthClient,
            cpr::Payload{ { "grant_type", "token_to_token" }, { "token_type", "eg1" }, { "access_token", Token } }
        );

        if (Cancelled) { return CANCELLED; }

        if (Response.status_code != 200) {
            return TTK_CODE_NOT_200;
        }

        auto RespJson = Http::ParseJson(Response);

        if (RespJson.HasParseError()) {
            return TTK_CODE_JSON;
        }

        OAuthResponse = std::move(RespJson);
        return SUCCESS;
    }
}
