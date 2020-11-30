#pragma once

#include "../../Http.h"

#include <atomic>
#include <future>
#include <string>

namespace EGL3::Web::Epic::Auth {
	class ExternalLogin {
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
			SUCCESS,
			CANCELLED,
			POLLING_JSON,
			REDIRECT_URL_EMPTY,
			LOGIN_STATE_NOT_FOUND,
			LOGIN_STATE_JSON,
			XSRF_1_NOT_204,
			LOGIN_REQUEST_NOT_200,
			XSRF_2_NOT_204,
			EXCH_REQUEST_NOT_200,
			EXCH_REQUEST_JSON
		};

		ExternalLogin(PlatformType Platform);

		~ExternalLogin();

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
}
