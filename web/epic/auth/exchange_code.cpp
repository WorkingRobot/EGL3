#include "exchange_code.h"

AuthExchangeCode::AuthExchangeCode(const cpr::Authentication& AuthClient, const std::string& ExchangeCode) : AuthClient(AuthClient), ExchangeCode(ExchangeCode), Cancelled(false)
{
	OAuthResponseFuture = std::async(std::launch::async, &AuthExchangeCode::RunOAuthResponseTask, this);
}

AuthExchangeCode::~AuthExchangeCode()
{
	Cancelled = true;
	if (OAuthResponseFuture.valid()) {
		OAuthResponseFuture.wait();
	}
}

const std::shared_future<AuthExchangeCode::ErrorCode>& AuthExchangeCode::GetOAuthResponseFuture() const
{
	return OAuthResponseFuture;
}

const rapidjson::Document& AuthExchangeCode::GetOAuthResponse() const
{
	return OAuthResponse;
}

AuthExchangeCode::ErrorCode AuthExchangeCode::RunOAuthResponseTask()
{
	auto Response = Http::Post(
		cpr::Url{ "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token" },
		AuthClient,
		cpr::Payload{ { "grant_type", "exchange_code" }, { "exchange_code", ExchangeCode } }
	);

	if (Cancelled) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_EXCH_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_EXCH_CODE_JSON;
	}

	OAuthResponse = std::move(RespJson);
	return ERROR_SUCCESS;
}
