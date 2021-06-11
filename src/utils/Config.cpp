#include "Config.h"

#include "KnownFolders.h"
#include "Log.h"

namespace EGL3::Utils::Config {
    void SetupFolders()
    {
        EGL3_VERIFY(!GetFolder().empty(), "Could not get config folder path");

        std::filesystem::create_directories(GetFolder());
        std::filesystem::create_directories(GetFolder() / "contentcache");
    }

    std::filesystem::path GetFolderInternal() {
        auto Path = Platform::GetKnownFolderPath(FOLDERID_RoamingAppData);
        if (Path.empty()) {
            return Path;
        }
        return Path / "EGL3";
    }

    const std::filesystem::path& GetFolder() {
        static std::filesystem::path Path = GetFolderInternal();
        return Path;
    }

    std::filesystem::path GetExePathInternal() {
        char* Path;
        if (_get_pgmptr(&Path) == 0) {
            return Path;
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