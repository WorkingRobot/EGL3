#pragma once

#include "Hosts.h"
#include "JsonParsing.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING // https://github.com/whoshuu/cpr/pull/473
#include <cpr/cpr.h>

namespace EGL3::Web::Http {
    namespace Detail {
        void DecorateSession(cpr::Session& Session);

        template<class... ArgTs>
        __forceinline cpr::Session GetSession(ArgTs&&... Args) {
            cpr::Session Session;
            DecorateSession(Session);
            cpr::priv::set_option(Session, std::forward<ArgTs>(Args)...);
            return Session;
        }
    }

    const cpr::UserAgent& GetUserAgent();

    // for handling everything http errors/logging/etc
    template<class... ArgTs>
    static cpr::Response Get(ArgTs&&... Args) {
        return Detail::GetSession(std::forward<ArgTs>(Args)...).Get();
    }

    template<class... ArgTs>
    static cpr::Response Post(ArgTs&&... Args) {
        return Detail::GetSession(std::forward<ArgTs>(Args)...).Post();
    }

    template<class... ArgTs>
    static cpr::Response Put(ArgTs&&... Args) {
        return Detail::GetSession(std::forward<ArgTs>(Args)...).Put();
    }

    template<class... ArgTs>
    static cpr::Response Patch(ArgTs&&... Args) {
        return Detail::GetSession(std::forward<ArgTs>(Args)...).Patch();
    }

    template<class... ArgTs>
    static cpr::Response Delete(ArgTs&&... Args) {
        return Detail::GetSession(std::forward<ArgTs>(Args)...).Delete();
    }

    template<class... ArgTs>
    static cpr::Response Head(ArgTs&&... Args) {
        return Detail::GetSession(std::forward<ArgTs>(Args)...).Head();
    }

    template<Host SelectedHost>
    static cpr::Url FormatUrl() {
        return GetHostUrl<SelectedHost>();
    }

    template<Host SelectedHost>
    static cpr::Url FormatUrl(const char* Input) {
        return std::format("{}{}", GetHostUrl<SelectedHost>(), Input);
    }

    template<Host SelectedHost, typename... Args>
    static cpr::Url FormatUrl(const char* Input, Args&&... FormatArgs) {
        return std::format("{}{}", GetHostUrl<SelectedHost>(), std::format(Input, std::move(FormatArgs)...));
    }

    static rapidjson::Document ParseJson(const cpr::Response& Response) {
        return Web::ParseJson(Response.text);
    }
}