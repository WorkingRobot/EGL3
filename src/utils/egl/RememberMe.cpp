#include "RememberMe.h"

#include "../Base64.h"
#include "../IniFile.h"
#include "../KnownFolders.h"
#include "../Platform.h"

namespace EGL3::Utils::EGL {
    RememberMe::RememberMe()
    {
        auto IniPath = GetIniPath();
        if (IniPath.empty()) {
            return;
        }

        IniFile Settings(IniPath);
        auto SectionPtr = Settings.GetSection("RememberMe");
        if (!SectionPtr) {
            return;
        }
        auto& Section = *SectionPtr;

        auto DataItr = Section.find("Data");
        if (DataItr == Section.end()) {
            return;
        }

        RememberMeData Data;
        if (!RememberMeData::Parse(Web::ParseJson(B64Decode(DataItr->second)), Data)) {
            return;
        }

        auto ProfileItr = std::find_if(Data.Profiles.begin(), Data.Profiles.end(), [](const RememberMeData::Profile& Profile) {
            return Profile.Region == "Prod";
        });
        if (ProfileItr == Data.Profiles.end()) {
            return;
        }

        Profile.emplace(std::move(*ProfileItr));
    }

    RememberMeData::Profile* RememberMe::GetProfile()
    {
        if (!Profile.has_value()) {
            return nullptr;
        }
        return &Profile.value();
    }

    std::filesystem::path RememberMe::GetIniPath()
    {
        auto Path = Platform::GetKnownFolderPath(FOLDERID_LocalAppData);
        if (Path.empty()) {
            return Path;
        }
        
        return Path / "EpicGamesLauncher" / "Saved" / "Config" / Platform::GetOSName() / "GameUserSettings.ini";
    }
}
