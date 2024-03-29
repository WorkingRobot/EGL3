#pragma once

#include "../../storage/models/PlayInfo.h"
#include "../../utils/SlotHolder.h"
#include "../ModuleList.h"
#include "../Login/Auth.h"
#include "Service.h"

namespace EGL3::Modules::Game {
    class PlayModule : public BaseModule {
    public:
        PlayModule(ModuleList& Ctx);

        Storage::Models::PlayInfo& OnPlayClicked(Storage::Models::InstalledGame& Game, bool WaitForExit);

        Utils::Callback<void(bool Playing)> OnStateUpdate;

    private:
        Login::AuthModule& Auth;
        ServiceModule& Service;

        Utils::SlotHolder SlotLogOutPreflight;

        bool WaitForExit;
        std::atomic<bool> PlayQueued;
        Glib::Dispatcher PlayQueuedDispatcher;
        std::unique_ptr<Storage::Models::PlayInfo> CurrentPlay;
    };
}