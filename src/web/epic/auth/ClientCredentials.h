#pragma once

#include "../../Http.h"

#include <atomic>
#include <string>

namespace EGL3::Web::Epic::Auth {
	class ClientCredentials {
	public:
		enum ErrorCode {
			SUCCESS,
			CANCELLED,
			EXCH_CODE_NOT_200,
			EXCH_CODE_JSON
		};

		ClientCredentials(const cpr::Authentication& AuthClient);

		~ClientCredentials();

		const std::shared_future<ErrorCode>& GetOAuthResponseFuture() const;

		const rapidjson::Document& GetOAuthResponse() const;

	private:
		ErrorCode RunOAuthResponseTask();

		std::atomic_bool Cancelled;

		std::shared_future<ErrorCode> OAuthResponseFuture;
		rapidjson::Document OAuthResponse;

		cpr::Authentication AuthClient;
	};
}
