#pragma once

#include "../../storage/models/UpdateInfo.h"
#include "../../storage/persistent/Store.h"
#include "../../utils/GladeBuilder.h"
#include "../../widgets/UpdateDisplay.h"
#include "../BaseModule.h"
#include "../ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class UpdaterModule : public BaseModule {
    public:
        UpdaterModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

        void QueueUpdate(Storage::Game::Archive&& Archive, Web::Epic::BPS::Manifest&& Manifest, size_t TaskCount);

    private:
        Storage::Persistent::Store& Storage;

        Gtk::Stack& MainStack;
        Gtk::Box& DownloadBox;
        Widgets::UpdateDisplay DownloadDisplay;

        std::vector<std::unique_ptr<Storage::Models::UpdateInfo>> Updates;
    };
}