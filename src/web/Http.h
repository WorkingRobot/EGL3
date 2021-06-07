#pragma once

#include "Hosts.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cpr/cpr.h>
#include <rapidjson/document.h>

//#define SUFFFIX_SSL
#define SUFFFIX_SSL cpr::Proxies{ {"https","localhost:8888"} }, cpr::VerifySsl{ false },

namespace EGL3::Web::Http {
    const cpr::UserAgent& GetUserAgent();

    // for handling everything http errors/logging/etc
    template<typename... Ts>
    static cpr::Response Get(Ts&&... ts) {
        return cpr::Get(GetUserAgent(), SUFFFIX_SSL std::move(ts)...);
    }

    template<typename... Ts>
    static cpr::Response Post(Ts&&... ts) {
        return cpr::Post(GetUserAgent(), SUFFFIX_SSL std::move(ts)...);
    }

    template<typename... Ts>
    static cpr::Response Delete(Ts&&... ts) {
        return cpr::Delete(GetUserAgent(), SUFFFIX_SSL std::move(ts)...);
    }

    template<typename... Ts>
    static cpr::Response Put(Ts&&... ts) {
        return cpr::Put(GetUserAgent(), SUFFFIX_SSL std::move(ts)...);
    }

    template<Host SelectedHost>
    static cpr::Url FormatUrl(const char* Input) {
        return std::format("{}{}", GetHostUrl<SelectedHost>(), Input);
    }

    template<Host SelectedHost, typename... Args>
    static cpr::Url FormatUrl(const char* Input, Args&&... FormatArgs) {
        return std::format("{}{}", GetHostUrl<SelectedHost>(), std::format(Input, std::move(FormatArgs)...));
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
}