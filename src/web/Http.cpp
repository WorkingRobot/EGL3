#include "Http.h"

#include "../utils/Platform.h"
#include "../utils/Version.h"

namespace EGL3::Web::Http {
    constexpr bool UseProxy = false;

    void Detail::DecorateSession(cpr::Session& Session)
    {
        Session.SetUserAgent(GetUserAgent());

        if constexpr (UseProxy) {
            Session.SetProxies({ {"https","localhost:8888"} });
            Session.SetVerifySsl(false);
        }
    }

    const cpr::UserAgent& GetUserAgent() {
        static cpr::UserAgent UserAgent = std::format("{}/{} {}/{}", Utils::Version::GetAppName(), Utils::Version::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion());

        return UserAgent;
    }
}