#pragma once

#include <filesystem>

namespace EGL3::Utils::Config {
    constexpr const char* GetLanguage() {
        return "en-US";
    }

    constexpr bool IsDefaultLanguage() {
        return true;
    }

    void SetupFolders();

    const std::filesystem::path& GetFolder();

    const std::filesystem::path& GetExePath();

    const std::filesystem::path& GetExeFolder();
}