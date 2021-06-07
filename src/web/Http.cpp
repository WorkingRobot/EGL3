#include "Http.h"

#include "../utils/Config.h"
#include "../utils/Platform.h"

namespace EGL3::Web::Http {
    const cpr::UserAgent& GetUserAgent() {
        static cpr::UserAgent UserAgent = std::format("{}/{} {}/{}", Utils::Config::GetAppName(), Utils::Config::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion());

        return UserAgent;
    }
}