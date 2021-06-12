#include "GameInfo.h"

#include "../../utils/formatters/Regex.h"
#include "../../utils/Platform.h"

#include <regex>

namespace EGL3::Modules::Game {
    GameInfoModule::GameInfoModule(ModuleList& Ctx) :
        Auth(Ctx.GetModule<Login::AuthModule>())
    {

    }

    Storage::Models::VersionData* GameInfoModule::GetVersionData(Storage::Game::GameId Id, bool ForceUpdate)
    {
        if (!ForceUpdate) {
            auto Itr = VersionData.find(Id);
            if (Itr != VersionData.end()) {
                return &Itr->second;
            }
        }
        
        auto Resp = GetVersionDataInternal(Id);
        if (!Resp.HasError()) {
            return &VersionData.emplace(Id, std::move(Resp.Get())).first->second;
        }
        return nullptr;
    }

    const std::vector<Web::Epic::Content::SdMeta::Data>* GameInfoModule::GetInstallOptions(Storage::Game::GameId Id, const std::string& Version, bool ForceUpdate)
    {
        std::string CatalogItemId, AppName;
        EGL3_VERIFY(GetCatalogInfo(Id, CatalogItemId, AppName), "Could not get catalog info");

        return Auth.GetClientLauncherContent().GetSdMetaData(CatalogItemId, AppName, Version);
    }

    bool GameInfoModule::GetCatalogInfo(Storage::Game::GameId Id, std::string& CatalogItemId, std::string& AppName)
    {
        switch (Id)
        {
        case Storage::Game::GameId::Fortnite:
            CatalogItemId = "4fe75bbc5a674f4f9b356b5c90567da5";
            AppName = "Fortnite";
            return true;
        default:
            CatalogItemId = "00000000000000000000000000000000";
            AppName = "Unknown";
            return false;
        }
    }

    bool GameInfoModule::GetGameName(Storage::Game::GameId Id, std::string& GameName)
    {
        switch (Id)
        {
        case Storage::Game::GameId::Fortnite:
            GameName = "Fortnite";
            return true;
        default:
            GameName = "Unknown";
            return false;
        }
    }

    bool GameInfoModule::ParseGameVersion(Storage::Game::GameId Id, const std::string& Version, uint64_t& VersionNum, std::string& VersionHR)
    {
        switch (Id)
        {
        case Storage::Game::GameId::Fortnite:
        {
            const static std::regex VersionRegex("\\+\\+Fortnite\\+Release-(\\d+)\\.(\\d+).*-CL-(\\d+)-.*");
            std::smatch Matches;
            if (std::regex_search(Version, Matches, VersionRegex)) {
                VersionHR = std::format("{}.{}", Matches[1], Matches[2]);
                VersionNum = -1;
                auto VersionStr = Matches[3].str();
                std::from_chars(VersionStr.c_str(), VersionStr.c_str() + VersionStr.size(), VersionNum);
                return true;
            }
            else {
                VersionHR = "Unknown";
                VersionNum = -1;
                return false;
            }
        }
        default:
            VersionHR = "Unknown";
            VersionNum = -1;
            return false;
        }
    }
    
    Web::Response<Storage::Models::VersionData> GameInfoModule::GetVersionDataInternal(Storage::Game::GameId Id)
    {
        switch (Id)
        {
        case Storage::Game::GameId::Fortnite:
        {
            std::string CatalogItemId, AppName;
            EGL3_VERIFY(GetCatalogInfo(Id, CatalogItemId, AppName), "Could not get catalog info");

            auto InfoResp = Auth.GetClientLauncher().GetDownloadInfo(Utils::Platform::GetOSName(), "Live", CatalogItemId, AppName);
            if (InfoResp.HasError()) {
                return InfoResp.GetError();
            }
            auto Element = InfoResp->GetElement(AppName);
            if (!Element) {
                return Web::ErrorData::Status::Failure;
            }

            Storage::Models::VersionData Ret{};
            Ret.Element = *Element;
            if (!ParseGameVersion(Id, Element->BuildVersion, Ret.VersionNum, Ret.VersionHR)) {
                return Web::ErrorData::Status::Failure;
            }
            return Ret;
        }
        default:
            return Web::ErrorData::Status::Failure;
        }
    }
}