#pragma once

#include "../../storage/models/PlayInfo.h"
#include "../ModuleList.h"
#include "../Login/Auth.h"
#include "Service.h"

namespace EGL3::Modules::Game {
    class PlayModule : public BaseModule {
    public:
        PlayModule(ModuleList& Ctx);

        Storage::Models::PlayInfo& OnPlayClicked(Storage::Models::InstalledGame& Game);

        Utils::Callback<void(bool Playing)> OnStateUpdate;

    private:
        Storage::Persistent::Store& Storage;
        Login::AuthModule& Auth;
        ServiceModule& Service;

        std::atomic<bool> PlayQueued;
        Glib::Dispatcher PlayQueuedDispatcher;
        std::unique_ptr<Storage::Models::PlayInfo> CurrentPlay;
    };
}