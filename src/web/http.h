#pragma once

#include "../utils/Format.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cpr/cpr.h>
#include <rapidjson/document.h>

#define WEB_SUFFIX_DATA
#define WEB_SUFFIX_DATA cpr::Proxies{ {"https","localhost:8888"} }, cpr::VerifySsl{ false },

namespace EGL3::Web {
	// static wrapper class for handling everything http errors/logging/etc
	class Http {
		// Add "cpr::Proxies{ {"https","localhost:8888"} }, cpr::VerifySsl{ false }, " for fiddler
	public:
		template<typename... Ts>
		static cpr::Response Get(Ts&&... ts) {
			return cpr::Get(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response Post(Ts&&... ts) {
			return cpr::Post(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response Delete(Ts&&... ts) {
			return cpr::Delete(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response Put(Ts&&... ts) {
			return cpr::Put(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}

		static cpr::Url FormatUrl(const char* Input) {
			return Input;
		}

		template<typename... Args>
		static cpr::Url FormatUrl(const char* Input, Args&&... FormatArgs) {
			return Utils::Format(Input, std::move(FormatArgs)...);
		}

		// Make sure to check validity with Json.HasParseError()
		// Use something similar to printf("%d @ %zu\n", Json.GetParseError(), Json.GetErrorOffset());
		static rapidjson::Document ParseJson(const std::string& Data) {
			rapidjson::Document Json;
			Json.Parse(Data.data(), Data.size());
			return Json;
		}

		static rapidjson::Document ParseJson(const cpr::Response& Response) {
			return ParseJson(Response.text);
		}

	private:
		template<typename... Ts>
		static cpr::Response GetSync(Ts&&... ts) {
			return cpr::Get(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response PostSync(Ts&&... ts) {
			return cpr::Post(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response DeleteSync(Ts&&... ts) {
			return cpr::Delete(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response PutSync(Ts&&... ts) {
			return cpr::Put(WEB_SUFFIX_DATA std::forward<decltype(ts)>(ts)...);
		}
	};
}