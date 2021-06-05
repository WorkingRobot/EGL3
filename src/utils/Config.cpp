#include "Config.h"

#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShlObj_core.h>

namespace EGL3::Utils::Config {
    std::filesystem::path GetFolderInternal() {
        std::filesystem::path Path;
        {
            CHAR PathBuf[MAX_PATH];
            if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, PathBuf) == S_OK) {
                Path = PathBuf;
            }
            else {
                Path = getenv("APPDATA");
            }
        }
        if (Path.empty()) {
            return Path;
        }
        return Path / "EGL3";
    }

    void SetupFolders()
    {
        EGL3_VERIFY(!GetFolder().empty(), "Could not get config folder path");

        std::filesystem::create_directories(GetFolder());
        std::filesystem::create_directories(GetFolder() / "contentcache");
    }

    const std::filesystem::path& GetFolder() {
        static std::filesystem::path Path = GetFolderInternal();
        return Path;
    }

    std::filesystem::path GetExePathInternal() {
        CHAR PathBuf[MAX_PATH];
        if (GetModuleFileName(NULL, PathBuf, MAX_PATH)) {
            return PathBuf;
        }
        return "";
    }

    const std::filesystem::path& GetExePath() {
        static std::filesystem::path Path = std::filesystem::absolute(GetExePathInternal());
        return Path;
    }

    const std::filesystem::path& GetExeFolder() {
        static std::filesystem::path Path = GetExePath().parent_path();
        return Path;
    }
}