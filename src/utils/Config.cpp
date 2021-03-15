#include "Config.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShlObj_core.h>

namespace EGL3::Utils::Config {
    std::filesystem::path GetConfigFolderInternal() {
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
        return Path / "EGL3";
    }

    const std::filesystem::path& GetConfigFolder() {
        static std::filesystem::path Path = GetConfigFolderInternal();
        return Path;
    }

    std::filesystem::path GetExePathInternal() {
        CHAR PathBuf[MAX_PATH];
        if (GetModuleFileName(NULL, PathBuf, MAX_PATH)) {
            return PathBuf;
        }
        return "";
    }

    const std::filesystem::path& GetExeFolder() {
        static std::filesystem::path Path = std::filesystem::absolute(GetExePathInternal().parent_path());
        return Path;
    }
}