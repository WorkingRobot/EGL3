#pragma once

#include "../../Http.h"

#include <atomic>
#include <string>

namespace EGL3::Web::Epic::Auth {
	class DeviceAuth {
	public:
		enum ErrorCode {
			ERROR_SUCCESS,
			ERROR_CANCELLED,
			ERROR_EXCH_CODE_NOT_200,
			ERROR_EXCH_CODE_JSON
		};

		DeviceAuth(const cpr::Authentication& AuthClient, const std::string& AccountId, const std::string& DeviceId, const std::string& Secret);

		~DeviceAuth();

		const std::shared_future<ErrorCode>& GetOAuthResponseFuture() const;

		const rapidjson::Document& GetOAuthResponse() const;

	private:
		ErrorCode RunOAuthResponseTask();

		std::atomic_bool Cancelled;

		std::shared_future<ErrorCode> OAuthResponseFuture;
		rapidjson::Document OAuthResponse;

		cpr::Authentication AuthClient;
		std::string AccountId;
		std::string DeviceId;
		std::string Secret;
	};
}
