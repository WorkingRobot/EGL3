#include "Http.h"

#include "../utils/Config.h"
#include "../utils/Platform.h"

namespace EGL3::Web::Http {
	const cpr::UserAgent& GetUserAgent() {
		static cpr::UserAgent UserAgent = Utils::Format("%s/%s %s/%s", Utils::Config::GetAppName(), Utils::Config::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion().c_str());

		return UserAgent;
	}
}