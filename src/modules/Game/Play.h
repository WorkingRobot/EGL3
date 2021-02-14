#pragma once

#include "../../storage/persistent/Store.h"
#include "../../storage/models/PlayInfo.h"
#include "../../storage/models/MountedDisk.h"
#include "../../utils/GladeBuilder.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../Authorization.h"

namespace EGL3::Modules::Game {
    class PlayModule : public BaseModule {
    public:
        PlayModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

        Storage::Models::PlayInfo& OnPlayClicked(const Storage::Models::InstalledGame& Game);

    private:
        Storage::Persistent::Store& Storage;
        Modules::AuthorizationModule& Auth;

        std::unique_ptr<Storage::Models::PlayInfo> CurrentPlay;

        std::optional<Storage::Models::MountedDisk> Disk;
    };
}