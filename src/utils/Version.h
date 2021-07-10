#pragma once

#include <stdint.h>

namespace EGL3::Utils::Version {
    const char* GetAppName();

    uint32_t GetMajorVersion();

    uint32_t GetMinorVersion();

    uint32_t GetPatchVersion();

    uint64_t GetVersionNum();

    const char* GetGitRevision();

    const char* GetGitBranch();

    const char* GetShortAppVersion();

    const char* GetAppVersion();
}