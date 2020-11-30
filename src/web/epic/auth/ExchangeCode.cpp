#include "ExchangeCode.h"

namespace EGL3::Web::Epic::Auth {
	ExchangeCode::ExchangeCode(const cpr::Authentication& AuthClient, const std::string& Code) : AuthClient(AuthClient), Code(Code), Cancelled(false)
	{
		OAuthResponseFuture = std::async(std::launch::async, &ExchangeCode::RunOAuthResponseTask, this);
	}

	ExchangeCode::~ExchangeCode()
	{
		Cancelled = true;
		if (OAuthResponseFuture.valid()) {
			OAuthResponseFuture.wait();
		}
	}

	const std::shared_future<ExchangeCode::ErrorCode>& ExchangeCode::GetOAuthResponseFuture() const
	{
		return OAuthResponseFuture;
	}

	const rapidjson::Document& ExchangeCode::GetOAuthResponse() const
	{
		return OAuthResponse;
	}

	ExchangeCode::ErrorCode ExchangeCode::RunOAuthResponseTask()
	{
		auto Response = Http::Post(
			cpr::Url{ "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token" },
			AuthClient,
			cpr::Payload{ { "grant_type", "exchange_code" }, { "exchange_code", Code } }
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
