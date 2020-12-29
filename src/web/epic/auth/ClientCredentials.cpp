#include "ClientCredentials.h"

namespace EGL3::Web::Epic::Auth {
	ClientCredentials::ClientCredentials(const cpr::Authentication& AuthClient) : AuthClient(AuthClient), Cancelled(false)
	{
		OAuthResponseFuture = std::async(std::launch::async, &ClientCredentials::RunOAuthResponseTask, this);
	}

	ClientCredentials::~ClientCredentials()
	{
		Cancelled = true;
		if (OAuthResponseFuture.valid()) {
			OAuthResponseFuture.wait();
		}
	}

	const std::shared_future<ClientCredentials::ErrorCode>& ClientCredentials::GetOAuthResponseFuture() const
	{
		return OAuthResponseFuture;
	}

	const rapidjson::Document& ClientCredentials::GetOAuthResponse() const
	{
		return OAuthResponse;
	}

	ClientCredentials::ErrorCode ClientCredentials::RunOAuthResponseTask()
	{
		auto Response = Http::Post(
			Http::FormatUrl<Host::Account>("oauth/token"),
			AuthClient,
			cpr::Payload{ { "grant_type", "client_credentials" } }
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
