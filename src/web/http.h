#pragma once

#include <cpr/cpr.h>
#include <rapidjson/document.h>

namespace EGL3::Web {
	// static wrapper class for handling everything http errors/logging/etc
	class Http {
	public:
		template<typename... Ts>
		static cpr::Response Get(Ts&&... ts) {
			return cpr::Get(std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response Post(Ts&&... ts) {
			return cpr::Post(std::forward<decltype(ts)>(ts)...);
		}

		template<typename... Ts>
		static cpr::Response Delete(Ts&&... ts) {
			return cpr::Delete(std::forward<decltype(ts)>(ts)...);
		}

		// Make sure to check validity with Json.HasParseError()
		// Use something similar to printf("%d @ %zu\n", Json.GetParseError(), Json.GetErrorOffset());
		static rapidjson::Document ParseJson(const std::string& Data) {
			rapidjson::Document Json;
			Json.Parse(Data.data(), Data.size());
			return Json;
		}

		// Make sure to check validity with Json.HasParseError()
		// Use something similar to printf("%d @ %zu\n", Json.GetParseError(), Json.GetErrorOffset());
		static rapidjson::Document ParseJson(const cpr::Response& Response) {
			return ParseJson(Response.text);
		}
	};
}