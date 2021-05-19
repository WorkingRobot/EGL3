#pragma once

#include "../../storage/persistent/Store.h"
#include "../../storage/models/PlayInfo.h"
#include "../../utils/GladeBuilder.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../Authorization.h"
#include "Service.h"

namespace EGL3::Modules::Game {
    class PlayModule : public BaseModule {
    public:
        PlayModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

        Storage::Models::PlayInfo& OnPlayClicked(Storage::Models::InstalledGame& Game);

        Utils::Callback<void(bool Playing)> OnStateUpdate;

    private:
        Storage::Persistent::Store& Storage;
        Modules::AuthorizationModule& Auth;
        ServiceModule& Service;

        std::atomic<bool> PlayQueued;
        std::unique_ptr<Storage::Models::PlayInfo> CurrentPlay;
    };
}