#pragma once

#include "../../storage/game/GameId.h"
#include "../../storage/models/VersionData.h"
#include "../ModuleList.h"
#include "../Login/Auth.h"

namespace EGL3::Modules::Game {
    class GameInfoModule : public BaseModule {
    public:
        GameInfoModule(ModuleList& Ctx);

        Storage::Models::VersionData* GetVersionData(Storage::Game::GameId Id, bool ForceUpdate = false);

        const std::vector<Web::Epic::Content::SdMeta::Data>* GetInstallOptions(Storage::Game::GameId Id, const std::string& Version, bool ForceUpdate = false);

        static bool GetCatalogInfo(Storage::Game::GameId Id, std::string& CatalogItemId, std::string& AppName);

        static bool GetGameName(Storage::Game::GameId Id, std::string& GameName);

        static bool ParseGameVersion(Storage::Game::GameId Id, const std::string& Version, uint64_t& VersionNum, std::string& VersionHR);

    private:
        Web::Response<Storage::Models::VersionData> GetVersionDataInternal(Storage::Game::GameId Id);

        std::unordered_map<Storage::Game::GameId, Storage::Models::VersionData> VersionData;
        std::unordered_map<Storage::Game::GameId, std::vector<Web::Epic::Content::SdMeta::Data>> InstallOptions;
        Login::AuthModule& Auth;
    };
}