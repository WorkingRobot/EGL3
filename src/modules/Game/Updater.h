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

        template<class... ArgsT>
        void QueueUpdate(ArgsT&&... Args) {
            EGL3_CONDITIONAL_LOG(Updates.empty(), LogLevel::Critical, "Hardcoded limit, only 1 update is allowed at a time");
            QueueUpdateInternal(Updates.emplace_back(std::make_unique<Storage::Models::UpdateInfo>(std::forward<ArgsT>(Args)...)));
        }

    private:
        void QueueUpdateInternal(std::unique_ptr<Storage::Models::UpdateInfo>& Info);

        Storage::Persistent::Store& Storage;

        Gtk::Stack& MainStack;
        Gtk::Box& DownloadBox;
        Widgets::UpdateDisplay DownloadDisplay;

        std::vector<std::unique_ptr<Storage::Models::UpdateInfo>> Updates;
    };
}