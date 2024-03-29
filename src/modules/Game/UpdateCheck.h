#pragma once

#include "../../storage/models/InstalledGame.h"
#include "../../storage/models/VersionData.h"
#include "../../utils/Callback.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../Login/Auth.h"
#include "GameInfo.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class UpdateCheckModule : public BaseModule {
    public:
        UpdateCheckModule(ModuleList& Ctx);

        ~UpdateCheckModule();

        void SetFrequency(std::chrono::seconds NewFrequency);

        std::chrono::seconds GetFrequency() const;

        const std::vector<Storage::Models::InstalledGame>& GetInstalledGames() const;

        std::vector<Storage::Models::InstalledGame>& GetInstalledGames();

        void FlushInstalledGames();

        // Will only be emitted for already installed games
        sigc::signal<void(Storage::Game::GameId Id, const Storage::Models::VersionData&)> OnUpdateAvailable;

    private:
        void CheckForUpdate(Storage::Game::GameId Id, uint64_t StoredVersion);

        void BackgroundTask();

        Login::AuthModule& Auth;
        AsyncFFModule& AsyncFF;
        GameInfoModule& GameInfo;

        using InstalledGamesSetting = Storage::Persistent::Setting<Utils::Crc32("InstalledGames"), std::vector<Storage::Models::InstalledGame>>;
        Storage::Persistent::SettingHolder<InstalledGamesSetting> InstalledGames;

        using UpdateFrequencySetting = Storage::Persistent::Setting<Utils::Crc32("UpdateFrequency"), std::chrono::seconds>;
        Storage::Persistent::SettingHolder<UpdateFrequencySetting> UpdateFrequency;

        std::mutex Mutex;
        std::condition_variable CV;
        bool Cancelled;
        std::future<void> Future;
    };
}