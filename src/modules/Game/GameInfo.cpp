#include "GameInfo.h"

#include <charconv>
#include <regex>

namespace EGL3::Modules::Game {
    GameInfoModule::GameInfoModule(ModuleList& Modules) :
        Auth(Modules.GetModule<AuthorizationModule>())
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

    bool GameInfoModule::ParseGameVersion(Storage::Game::GameId Id, const std::string& Version, uint64_t& VersionNum, std::string& VersionHR)
    {
        switch (Id)
        {
        case Storage::Game::GameId::Fortnite:
        {
            const static std::regex VersionRegex("\\+\\+Fortnite\\+Release-(\\d+)\\.(\\d+).*-CL-(\\d+)-.*");
            std::smatch Matches;
            if (std::regex_search(Version, Matches, VersionRegex)) {
                VersionHR = Utils::Format("%s.%s", Matches[1].str().c_str(), Matches[2].str().c_str());
                VersionNum = -1;
                auto VersionStr = Matches[3].str();
                std::from_chars(VersionStr.c_str(), VersionStr.c_str() + VersionStr.size(), VersionNum);
                return true;
            }
            else {
                return false;
            }
        }
        default:
            return false;
        }
    }
    
    Web::Response<Storage::Models::VersionData> GameInfoModule::GetVersionDataInternal(Storage::Game::GameId Id)
    {
        if (!Auth.IsLoggedIn()) {
            return Web::ErrorData::Status::InvalidToken;
        }

        switch (Id)
        {
        case Storage::Game::GameId::Fortnite:
        {
            auto InfoResp = Auth.GetClientLauncher().GetDownloadInfo("Windows", "Live", "4fe75bbc5a674f4f9b356b5c90567da5", "Fortnite");
            if (InfoResp.HasError()) {
                return InfoResp.GetError();
            }
            auto Element = InfoResp->GetElement("Fortnite");
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