#include "UpdateCheck.h"

namespace EGL3::Modules::Game {
    UpdateCheckModule::UpdateCheckModule(ModuleList& Ctx) :
        Storage(Ctx.GetStorage()),
        Auth(Ctx.GetModule<Login::AuthModule>()),
        AsyncFF(Ctx.GetModule<AsyncFFModule>()),
        GameInfo(Ctx.GetModule<GameInfoModule>()),
        Cancelled(false),
        Future(std::async(std::launch::async, &UpdateCheckModule::BackgroundTask, this))
    {
        
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

    std::chrono::seconds UpdateCheckModule::GetFrequency() const
    {
        auto& Freq = Storage.Get(Storage::Persistent::Key::UpdateFrequency);
        if (Freq < std::chrono::seconds(10)) {
            Freq = std::chrono::seconds(10);
            Storage.Flush();
        }
        return Freq;
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
            auto& InstalledGames = Storage.Get(Storage::Persistent::Key::InstalledGames);
            for (auto& Game : InstalledGames) {
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