#pragma once

#include <filesystem>

namespace EGL3::Utils::Config {
    constexpr const char* GetAppName() {
        return "EGL3";
    }

    constexpr uint32_t GetMajorVersion() {
        return CONFIG_VERSION_MAJOR;
    }

    constexpr uint32_t GetMinorVersion() {
        return CONFIG_VERSION_MINOR;
    }

    constexpr uint32_t GetPatchVersion() {
        return CONFIG_VERSION_PATCH;
    }

    constexpr const char* GetGitRevision() {
        return CONFIG_VERSION_HASH;
    }

    constexpr const char* GetGitBranch() {
        return CONFIG_VERSION_BRANCH;
    }

    constexpr const char* GetShortAppVersion() {
        return CONFIG_VERSION_SHORT;
    }

    constexpr const char* GetAppVersion() {
        return CONFIG_VERSION_LONG;
    }

    void SetupFolders();

    const std::filesystem::path& GetFolder();

    const std::filesystem::path& GetExeFolder();
}