#pragma once

#include <atomic>
#include <future>
#include <string>

class AuthExternalLogin {
public:
	enum PlatformType {
		AUTH_EPIC, // very clearly exists but doesn't work (redirects to your account settings in browser)
		AUTH_FACEBOOK,
		AUTH_GOOGLE,
		AUTH_XBOX,
		AUTH_PSN,
		AUTH_NINTENDO,
		AUTH_STEAM,
		AUTH_APPLE,
	};

	enum ErrorCode {
		ERROR_SUCCESS,
		ERROR_CANCELLED,
		ERROR_POLLING_JSON,
		ERROR_REDIRECT_URL_EMPTY,
		ERROR_LOGIN_STATE_NOT_FOUND,
		ERROR_LOGIN_STATE_JSON,
		ERROR_XSRF_1_NOT_204,
		ERROR_LOGIN_REQUEST_NOT_200,
		ERROR_XSRF_2_NOT_204,
		ERROR_EXCH_REQUEST_NOT_200,
		ERROR_EXCH_REQUEST_JSON
	};

	AuthExternalLogin(PlatformType Platform);

	~AuthExternalLogin();

	const std::shared_future<ErrorCode>& GetBrowserUrlFuture() const;

	const std::string& GetBrowserUrl() const;

	const std::shared_future<ErrorCode>& GetExchangeCodeFuture() const;

	const std::string& GetExchangeCode() const;

private:
	static const char* GetInternalPlatformName(PlatformType Platform);

	ErrorCode RunBrowserUrlTask();
	ErrorCode RunExchangeCodeTask();

	std::atomic_bool Cancelled;

	std::shared_future<ErrorCode> BrowserUrlFuture;
	std::string BrowserUrl;

	std::shared_future<ErrorCode> ExchangeCodeFuture;
	std::string ExchangeCode;

	PlatformType Platform;

	std::string LoginRequestId;
};
