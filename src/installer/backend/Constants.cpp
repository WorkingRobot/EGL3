#include "Constants.h"

namespace EGL3::Installer::Backend {
    std::string_view GetInstallVersion()
    {
        static constexpr std::string_view InstallVersion = INSTALLER_VERSION;
        return InstallVersion;
    }

    std::wstring_view GetInstallVersionWide()
    {
#define WIDE_IMPL(t) L ## t
#define WIDE(t) WIDE_IMPL(t)
        static constexpr std::wstring_view InstallVersion = WIDE(INSTALLER_VERSION);
        return InstallVersion;
    }
}