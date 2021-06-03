#pragma once

#include "../../storage/models/VersionData.h"
#include "../ModuleList.h"
#include "../Login/Auth.h"

namespace EGL3::Modules::Game {
    class GameInfoModule : public BaseModule {
    public:
        GameInfoModule(ModuleList& Ctx);

        Storage::Models::VersionData* GetVersionData(Storage::Game::GameId Id, bool ForceUpdate = false);

        const std::vector<Web::Epic::Content::SdMeta::Data>* GetInstallOptions(Storage::Game::GameId Id, const std::string& Version, bool ForceUpdate = false);

        static bool ParseGameVersion(Storage::Game::GameId Id, const std::string& Version, std::string& GameName, uint64_t& VersionNum, std::string& VersionHR);

    private:
        Web::Response<Storage::Models::VersionData> GetVersionDataInternal(Storage::Game::GameId Id);

        std::unordered_map<Storage::Game::GameId, Storage::Models::VersionData> VersionData;
        std::unordered_map<Storage::Game::GameId, std::vector<Web::Epic::Content::SdMeta::Data>> InstallOptions;
        Login::AuthModule& Auth;
    };
}