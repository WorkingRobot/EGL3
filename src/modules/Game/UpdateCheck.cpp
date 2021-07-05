#include "UpdateCheck.h"

namespace EGL3::Modules::Game {

    constexpr auto MinimumUpdateFrequency = std::chrono::seconds(30);
    constexpr auto MaximumUpdateFrequency = std::chrono::minutes(30);

    UpdateCheckModule::UpdateCheckModule(ModuleList& Ctx) :
        Auth(Ctx.GetModule<Login::AuthModule>()),
        AsyncFF(Ctx.GetModule<AsyncFFModule>()),
        GameInfo(Ctx.GetModule<GameInfoModule>()),
        UpdateFrequency(Ctx.Get<UpdateFrequencySetting>()),
        Cancelled(false),
        Future(std::async(std::launch::async, &UpdateCheckModule::BackgroundTask, this))
    {
        SetFrequency(*UpdateFrequency);
    }

    UpdateCheckModule::~UpdateCheckModule()
    {
        {
            std::unique_lock Lock(Mutex);
            Cancelled = true;
        }
        CV.notify_all();
        Future.get();
    }

    void UpdateCheckModule::SetInstalledGames(std::vector<Storage::Models::InstalledGame>& InstalledGames)
    {
        InstalledGamesPtr.store(&InstalledGames);
    }

    void UpdateCheckModule::SetFrequency(std::chrono::seconds NewFrequency)
    {
        if (NewFrequency < MinimumUpdateFrequency) {
            NewFrequency = MinimumUpdateFrequency;
        }

        UpdateFrequency = std::chrono::duration_cast<std::chrono::seconds>(std::clamp<std::chrono::steady_clock::duration>(NewFrequency, MinimumUpdateFrequency, MaximumUpdateFrequency));
        UpdateFrequency.Flush();
    }

    std::chrono::seconds UpdateCheckModule::GetFrequency() const
    {
        return *UpdateFrequency;
    }

    void UpdateCheckModule::CheckForUpdate(Storage::Game::GameId Id, uint64_t StoredVersion)
    {
        auto Data = GameInfo.GetVersionData(Id, true);
        if (!Data) {
            return;
        }
        if (Data->VersionNum == StoredVersion) { // Currently updating, etc.
            return;
        }
        OnUpdateAvailable(Id, *Data);
    }

    void UpdateCheckModule::BackgroundTask()
    {
        std::unique_lock Lock(Mutex);
        do {
            auto InstalledGames = InstalledGamesPtr.load();
            if (InstalledGames == nullptr) {
                continue;
            }
            for (auto& Game : *InstalledGames) {
                if (Game.IsValid() && !Game.IsArchiveOpen()) {
                    if (auto HeaderPtr = Game.GetHeader()) {
                        if (!HeaderPtr->GetUpdateInfo().IsUpdating) {
                            CheckForUpdate(HeaderPtr->GetGameId(), HeaderPtr->GetVersionNum());
                        }
                    }
                }
            }
        } while (!CV.wait_for(Lock, GetFrequency(), [this]() { return Cancelled; }));
    }
}