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

    uint64_t GetVersionNum()
    {
        return (uint64_t(GetMajorVersion()) << 16) | (uint64_t(GetMinorVersion()) << 8) | (uint64_t(GetPatchVersion()) << 0);
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