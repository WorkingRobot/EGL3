#include "ExternalLogin.h"

#include "../../../utils/Base64.h"
#include "../../../utils/Hex.h"
#include "../../../utils/RandGuid.h"
#include "../../../utils/UrlEncode.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Epic::Auth {
	ExternalLogin::ExternalLogin(PlatformType Platform) : Platform(Platform), Cancelled(false)
	{
		BrowserUrlFuture = std::async(std::launch::async, &ExternalLogin::RunBrowserUrlTask, this);
	}

	ExternalLogin::~ExternalLogin()
	{
		Cancelled = true;
		if (BrowserUrlFuture.valid()) {
			BrowserUrlFuture.wait();
		}
		if (ExchangeCodeFuture.valid()) {
			ExchangeCodeFuture.wait();
		}
	}

	const std::shared_future<ExternalLogin::ErrorCode>& ExternalLogin::GetBrowserUrlFuture() const
	{
		return BrowserUrlFuture;
	}

	const std::string& ExternalLogin::GetBrowserUrl() const
	{
		return BrowserUrl;
	}

	const std::shared_future<ExternalLogin::ErrorCode>& ExternalLogin::GetExchangeCodeFuture() const
	{
		return ExchangeCodeFuture;
	}

	const std::string& ExternalLogin::GetExchangeCode() const
	{
		return ExchangeCode;
	}

	const char* ExternalLogin::GetInternalPlatformName(PlatformType Platform)
	{
		switch (Platform)
		{
		case AUTH_EPIC:
			return "epic";
		case AUTH_FACEBOOK:
			return "facebook";
		case AUTH_GOOGLE:
			return "google";
		case AUTH_XBOX:
			return "xbl";
		case AUTH_PSN:
			return "psn";
		case AUTH_NINTENDO:
			return "nintendo";
		case AUTH_STEAM:
			return "steam";
		case AUTH_APPLE:
			return "apple";
		default:
			return "null";
		}
	}

	ExternalLogin::ErrorCode ExternalLogin::RunBrowserUrlTask()
	{
		uint8_t TrackingUuid[16];
		uint8_t LoginRequestId[16];

		Utils::GenerateRandomGuid(TrackingUuid);
		Utils::GenerateRandomGuid(LoginRequestId);
		this->LoginRequestId = Utils::ToHex<16>(LoginRequestId);

		if (Cancelled) { return CANCELLED; }

		rapidjson::StringBuffer ExtLoginStateBuffer;
		{
			rapidjson::Writer<rapidjson::StringBuffer> Writer(ExtLoginStateBuffer);

			Writer.StartObject();

			Writer.Key("trackingUuid");
			Writer.String(Utils::ToHex<16>(TrackingUuid).c_str());

			Writer.Key("loginRequestId");
			Writer.String(this->LoginRequestId.c_str());

			Writer.Key("authCode");
			Writer.Null();

			Writer.Key("isPopup");
			Writer.Bool(true);

			Writer.EndObject();
		}

		BrowserUrl =
			std::string("https://epicgames.com/id/login/") +
			GetInternalPlatformName(Platform) +
			"/forward?extLoginState=" +
			Utils::UrlEncode(Utils::UrlEncode( // double url encoded for some reason
				Utils::B64Encode((uint8_t*)ExtLoginStateBuffer.GetString(), ExtLoginStateBuffer.GetSize())
			)) +
			"&lang=en";

		ExchangeCodeFuture = std::async(std::launch::async, &ExternalLogin::RunExchangeCodeTask, this);

		return SUCCESS;
	}

	ExternalLogin::ErrorCode ExternalLogin::RunExchangeCodeTask()
	{
		if (Cancelled) { return CANCELLED; }

		std::string RedirectUrl;
		{
			auto NextSleep = std::chrono::steady_clock::now() + std::chrono::seconds(3);
			while (!Cancelled) {
				std::this_thread::sleep_until(NextSleep);
				if (Cancelled) { return CANCELLED; }
				NextSleep += std::chrono::seconds(3);

				auto Response = Http::Get(
					cpr::Url{ "https://www.epicgames.com/id/api/polling/" + LoginRequestId }
				);
				if (Cancelled) { return CANCELLED; }

				if (Response.status_code != 200) {
					// log error maybe?
					continue;
				}

				if (Response.text != "null") {
					auto RespJson = Http::ParseJson(Response);

					if (RespJson.HasParseError()) {
						// printf("polling json error %d @ %zu\n", RespJson.GetParseError(), RespJson.GetErrorOffset());
						return POLLING_JSON;
					}

					// RespJson["map"]["ipAddress"] also exists (format of "8.8.8.8")
					auto& RedirectUrlToken = RespJson["map"]["redirectUrl"];
					RedirectUrl = std::string(RedirectUrlToken.GetString(), RedirectUrlToken.GetStringLength());
					break;
				}
			}
		}

		if (RedirectUrl.empty()) {
			return REDIRECT_URL_EMPTY;
		}

		// I do the minimum number of requests to login. The launcher may send a bunch of extra requests/headers, but
		// that doesn't feel correct/moral/smart to emulate every request to look exactly like EGL
		// Sidenote: after more testing and snooping around, you can actually use the code from the extLoginState and
		// pass it to grant_type=external_auth as the external_auth_token and get the exchange code or whatever that way

		std::string AuthCode;
		{
			auto LoginStateStart = RedirectUrl.find("extLoginState=");
			if (LoginStateStart == std::string::npos) {
				return LOGIN_STATE_NOT_FOUND;
			}
			LoginStateStart += 14; // length of extLoginState=

			auto LoginStateEnd = RedirectUrl.find("&", LoginStateStart);
			if (LoginStateEnd == std::string::npos) {
				LoginStateEnd = RedirectUrl.size();
			}
			LoginStateEnd -= LoginStateStart;

			// trackingUuid : string		from request
			// loginRequestId : string		from request
			// isWeb : bool					true
			// ip : string					format: 8.8.8.8
			// id : string					some guid?
			// code : string				pass this on
			auto RespJson = Http::ParseJson(
				Utils::B64Decode(
					Utils::UrlDecode(Utils::UrlDecode( // double url encoded for some reason
						std::string(RedirectUrl.c_str() + LoginStateStart, LoginStateEnd) // Take only the extLoginState parameter
					))
				)
			);

			if (RespJson.HasParseError()) {
				return LOGIN_STATE_JSON;
			}

			auto& AuthCodeToken = RespJson["code"];
			AuthCode = std::string(AuthCodeToken.GetString(), AuthCodeToken.GetStringLength());
		}

		if (Cancelled) { return CANCELLED; }

		rapidjson::StringBuffer AuthCodeBuffer;
		{
			rapidjson::Writer<rapidjson::StringBuffer> Writer(AuthCodeBuffer);

			Writer.StartObject();

			Writer.Key("code");
			Writer.String(AuthCode.data(), AuthCode.size());

			Writer.EndObject();
		}

		if (Cancelled) { return CANCELLED; }

		auto XSRFTokenRequest = Http::Get(
			cpr::Url{ "https://www.epicgames.com/id/api/csrf" }
		);

		if (XSRFTokenRequest.status_code != 204) {
			return XSRF_1_NOT_204;
		}

		if (Cancelled) { return CANCELLED; }

		auto LoginRequest = Http::Post(
			cpr::Url{ std::string("https://www.epicgames.com/id/api/external/") + GetInternalPlatformName(Platform) + "/login" },
			cpr::Body{ AuthCodeBuffer.GetString(), AuthCodeBuffer.GetSize() },
			cpr::Header{ {"Content-Type","application/json" }, { "X-XSRF-TOKEN", XSRFTokenRequest.cookies["XSRF-TOKEN"] } },
			XSRFTokenRequest.cookies
		);

		if (Cancelled) { return CANCELLED; }

		if (LoginRequest.status_code != 200) {
			return LOGIN_REQUEST_NOT_200;
		}

		XSRFTokenRequest = Http::Get(
			cpr::Url{ "https://www.epicgames.com/id/api/csrf" },
			LoginRequest.cookies
		);

		if (XSRFTokenRequest.status_code != 204) {
			return XSRF_2_NOT_204;
		}

		if (Cancelled) { return CANCELLED; }

		auto ExchangeCodeRequest = Http::Post(
			cpr::Url{ "https://www.epicgames.com/id/api/exchange/generate" },
			cpr::Header{ { "X-XSRF-TOKEN", XSRFTokenRequest.cookies["XSRF-TOKEN"] } },
			cpr::Body{ },
			XSRFTokenRequest.cookies
		);

		if (ExchangeCodeRequest.status_code != 200) {
			return EXCH_REQUEST_NOT_200;
		}

		if (Cancelled) { return CANCELLED; }

		rapidjson::Document RespJson;
		RespJson.Parse(ExchangeCodeRequest.text.data(), ExchangeCodeRequest.text.size());

		if (Cancelled) { return CANCELLED; }

		if (RespJson.HasParseError()) {
			// printf("exch json error %d @ %zu\n", RespJson.GetParseError(), RespJson.GetErrorOffset());
			return EXCH_REQUEST_JSON;
		}

		auto& ExchangeCodeToken = RespJson["code"];
		ExchangeCode = std::string(ExchangeCodeToken.GetString(), ExchangeCodeToken.GetStringLength());
		return SUCCESS;
	}
}