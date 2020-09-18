#include "token_to_token.h"

namespace EGL3::Web::Epic::Auth {
	AuthTokenToToken::AuthTokenToToken(const cpr::Authentication& AuthClient, const std::string& Token) : AuthClient(AuthClient), Token(Token), Cancelled(false)
	{
		OAuthResponseFuture = std::async(std::launch::async, &AuthTokenToToken::RunOAuthResponseTask, this);
	}

	AuthTokenToToken::~AuthTokenToToken()
	{
		Cancelled = true;
		if (OAuthResponseFuture.valid()) {
			OAuthResponseFuture.wait();
		}
	}

	const std::shared_future<AuthTokenToToken::ErrorCode>& AuthTokenToToken::GetOAuthResponseFuture() const
	{
		return OAuthResponseFuture;
	}

	const rapidjson::Document& AuthTokenToToken::GetOAuthResponse() const
	{
		return OAuthResponse;
	}

	AuthTokenToToken::ErrorCode AuthTokenToToken::RunOAuthResponseTask()
	{
		auto Response = Http::Post(
			cpr::Url{ "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token" },
			AuthClient,
			cpr::Payload{ { "grant_type", "token_to_token" }, { "access_token", Token } }
		);

		if (Cancelled) { return ERROR_CANCELLED; }

		if (Response.status_code != 200) {
			return ERROR_TTK_CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ERROR_TTK_CODE_JSON;
		}

		OAuthResponse = std::move(RespJson);
		return ERROR_SUCCESS;
	}
}
