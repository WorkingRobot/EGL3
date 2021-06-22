#include "Http.h"

#include "../utils/Platform.h"
#include "../utils/Version.h"

namespace EGL3::Web::Http {
    const cpr::UserAgent& GetUserAgent() {
        static cpr::UserAgent UserAgent = std::format("{}/{} {}/{}", Utils::Version::GetAppName(), Utils::Version::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion());

        return UserAgent;
    }
}