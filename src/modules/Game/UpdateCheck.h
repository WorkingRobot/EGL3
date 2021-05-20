#pragma once

#include "../../storage/models/InstalledGame.h"
#include "../../storage/models/VersionData.h"
#include "../../storage/persistent/Store.h"
#include "../../utils/Callback.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../Authorization.h"
#include "GameInfo.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class UpdateCheckModule : public BaseModule {
    public:
        UpdateCheckModule(ModuleList& Modules, Storage::Persistent::Store& Storage);

        ~UpdateCheckModule();

        // Will only be emitted for already installed games
        sigc::signal<void(Storage::Game::GameId Id, const Storage::Models::VersionData&)> OnUpdateAvailable;

    private:
        std::chrono::seconds GetFrequency() const;

        void CheckForUpdate(Storage::Game::GameId Id, uint64_t StoredVersion);

        void BackgroundTask();

        Storage::Persistent::Store& Storage;
        Modules::AuthorizationModule& Auth;
        Modules::AsyncFFModule& AsyncFF;
        GameInfoModule& GameInfo;

        std::mutex Mutex;
        std::condition_variable CV;
        bool Cancelled;
        std::future<void> Future;
    };
}