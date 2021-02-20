#pragma once

#include "../../storage/models/InstalledGame.h"
#include "../../storage/models/VersionData.h"
#include "../../storage/persistent/Store.h"
#include "../../utils/Callback.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../Authorization.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class UpdateCheckModule : public BaseModule {
    public:
        UpdateCheckModule(ModuleList& Modules, Storage::Persistent::Store& Storage);

        ~UpdateCheckModule();

        Utils::Callback<Storage::Models::VersionData(Storage::Game::GameId)> RequestVersionData;

        sigc::signal<void(const Storage::Models::VersionData&)> OnUpdateAvailable;

    private:
        void CheckForUpdate(Storage::Game::GameId Id, uint64_t StoredVersion);

        void BackgroundTask();

        Storage::Persistent::Store& Storage;
        Modules::AuthorizationModule& Auth;
        Modules::AsyncFFModule& AsyncFF;

        std::mutex Mutex;
        std::condition_variable CV;
        bool Cancelled;
        std::future<void> Future;
    };
}