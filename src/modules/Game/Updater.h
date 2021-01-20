#pragma once

#include "../../storage/models/UpdateInfo.h"
#include "../../storage/persistent/Store.h"
#include "../../utils/GladeBuilder.h"
#include "../BaseModule.h"
#include "../ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class UpdaterModule : public BaseModule {
    public:
        UpdaterModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

        void QueueUpdate(Storage::Models::UpdateInfo& UpdateInfo);

    private:
        Storage::Persistent::Store& Storage;

        std::vector<Storage::Models::UpdateInfo> Updates;
    };
}