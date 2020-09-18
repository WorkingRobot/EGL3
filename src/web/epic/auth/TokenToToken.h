#pragma once

#include "../../Http.h"

#include <atomic>
#include <string>

namespace EGL3::Web::Epic::Auth {
	class TokenToToken {
	public:
		enum ErrorCode {
			ERROR_SUCCESS,
			ERROR_CANCELLED,
			ERROR_TTK_CODE_NOT_200,
			ERROR_TTK_CODE_JSON
		};

		TokenToToken(const cpr::Authentication& AuthClient, const std::string& Token);

		~TokenToToken();

		const std::shared_future<ErrorCode>& GetOAuthResponseFuture() const;

		const rapidjson::Document& GetOAuthResponse() const;

	private:
		ErrorCode RunOAuthResponseTask();

		std::atomic_bool Cancelled;

		std::shared_future<ErrorCode> OAuthResponseFuture;
		rapidjson::Document OAuthResponse;

		cpr::Authentication AuthClient;
		std::string Token;
	};
}