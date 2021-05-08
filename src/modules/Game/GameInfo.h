#pragma once

#include "../../storage/models/VersionData.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../Authorization.h"

namespace EGL3::Modules::Game {
    class GameInfoModule : public BaseModule {
    public:
        GameInfoModule(ModuleList& Modules);

        Storage::Models::VersionData* GetVersionData(Storage::Game::GameId Id, bool ForceUpdate = false);

        static bool ParseGameVersion(Storage::Game::GameId Id, const std::string& Version, uint64_t& VersionNum, std::string& VersionHR);

    private:
        Web::Response<Storage::Models::VersionData> GetVersionDataInternal(Storage::Game::GameId Id);

        std::unordered_map<Storage::Game::GameId, Storage::Models::VersionData> VersionData;
        AuthorizationModule& Auth;
    };
}