#pragma once

#include "../../Http.h"

#include <atomic>
#include <chrono>
#include <string>

namespace EGL3::Web::Epic::Auth {
	class DeviceCode {
	public:
		enum ErrorCode {
			SUCCESS,
			CANCELLED,
			CLIENT_CREDS_NOT_200,
			CLIENT_CREDS_JSON,
			DEVICE_AUTH_NOT_200,
			DEVICE_AUTH_JSON,
			EXPIRED,
			DEVICE_CODE_JSON
		};

		DeviceCode(const cpr::Authentication& AuthClient);

		~DeviceCode();

		const std::shared_future<ErrorCode>& GetBrowserUrlFuture() const;

		const std::string& GetBrowserUrl() const;

		const std::shared_future<ErrorCode>& GetOAuthResponseFuture() const;

		const rapidjson::Document& GetOAuthResponse() const;

	private:
		ErrorCode RunBrowserUrlTask();
		ErrorCode RunOAuthResponseTask();

		std::atomic_bool Cancelled;

		std::shared_future<ErrorCode> BrowserUrlFuture;
		std::string BrowserUrl;

		std::shared_future<ErrorCode> OAuthResponseFuture;
		rapidjson::Document OAuthResponse;

		cpr::Authentication AuthClient;

		std::string Code;
		std::chrono::steady_clock::time_point ExpiresAt;
		std::chrono::seconds RefreshInterval;
	};
}