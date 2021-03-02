#pragma once

#include "../../storage/models/VersionData.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../Authorization.h"

namespace EGL3::Modules::Game {
    class GameInfoModule : public BaseModule {
    public:
        GameInfoModule(ModuleList& Modules);

        Web::Response<Storage::Models::VersionData> GetVersionData(Storage::Game::GameId Id);

        static bool ParseGameVersion(Storage::Game::GameId Id, const std::string& Version, uint64_t& VersionNum, std::string& VersionHR);

    private:
        AuthorizationModule& Auth;
    };
}