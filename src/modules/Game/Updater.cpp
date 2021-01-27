#include "Updater.h"

namespace EGL3::Modules::Game {
    UpdaterModule::UpdaterModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        MainStack(Builder.GetWidget<Gtk::Stack>("MainStack")),
        DownloadBox(Builder.GetWidget<Gtk::Box>("DownloadContainer"))
    {
        DownloadBox.pack_start(DownloadDisplay, true, true);
    }

    void UpdaterModule::QueueUpdateInternal(std::unique_ptr<Storage::Models::UpdateInfo>& Info)
    {
        Info->StatsUpdate.Set([this](const Storage::Models::UpdateStatsInfo& Info) { DownloadDisplay.Update(Info); });
        Info->Begin();
        MainStack.set_visible_child(DownloadBox);
    }
}