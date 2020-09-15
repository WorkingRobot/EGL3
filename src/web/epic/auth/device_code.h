#pragma once

#include "../../http.h"

#include <atomic>
#include <chrono>
#include <string>

class AuthDeviceCode {
public:
	enum ErrorCode {
		ERROR_SUCCESS,
		ERROR_CANCELLED,
		ERROR_CLIENT_CREDS_NOT_200,
		ERROR_CLIENT_CREDS_JSON,
		ERROR_DEVICE_AUTH_NOT_200,
		ERROR_DEVICE_AUTH_JSON,
		ERROR_EXPIRED,
		ERROR_DEVICE_CODE_JSON
	};

	AuthDeviceCode(const cpr::Authentication& AuthClient);

	~AuthDeviceCode();

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

	std::string DeviceCode;
	std::chrono::steady_clock::time_point ExpiresAt;
	std::chrono::seconds RefreshInterval;
};