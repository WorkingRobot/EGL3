#pragma once

#include "../../storage/game/Archive.h"
#include "../../storage/persistent/Store.h"
#include "../../utils/GladeBuilder.h"
#include "../BaseModule.h"
#include "../ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class UpdaterModule : public BaseModule {
    public:
        UpdaterModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

        void CheckForUpdate();

    private:
        std::optional<Storage::Game::Archive> Archive;

        Storage::Persistent::Store& Storage;
    };
}