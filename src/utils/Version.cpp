#include "Version.h"

namespace EGL3::Utils::Version {
    const char* GetAppName()
    {
        return "EGL3";
    }

    uint32_t GetMajorVersion()
    {
        return CONFIG_VERSION_MAJOR;
    }

    uint32_t GetMinorVersion()
    {
        return CONFIG_VERSION_MINOR;
    }

    uint32_t GetPatchVersion()
    {
        return CONFIG_VERSION_PATCH;
    }

    const char* GetGitRevision()
    {
        return CONFIG_VERSION_HASH;
    }

    const char* GetGitBranch()
    {
        return CONFIG_VERSION_BRANCH;
    }

    const char* GetShortAppVersion()
    {
        return CONFIG_VERSION_SHORT;
    }

    const char* GetAppVersion()
    {
        return CONFIG_VERSION_LONG;
    }
}