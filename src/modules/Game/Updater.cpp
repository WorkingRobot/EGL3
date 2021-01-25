#include "Updater.h"

namespace EGL3::Modules::Game {
    UpdaterModule::UpdaterModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        MainStack(Builder.GetWidget<Gtk::Stack>("MainStack")),
        DownloadBox(Builder.GetWidget<Gtk::Box>("DownloadContainer"))
    {
        DownloadBox.pack_start(DownloadDisplay, true, true);
    }

    void UpdaterModule::QueueUpdate(Storage::Game::Archive&& Archive, Web::Epic::BPS::Manifest&& Manifest, size_t TaskCount)
    {
        EGL3_CONDITIONAL_LOG(Updates.empty(), LogLevel::Critical, "Hardcoded limit, only 1 update is allowed at a time");
        auto& Info = Updates.emplace_back(std::make_unique<Storage::Models::UpdateInfo>(std::move(Archive), std::move(Manifest), TaskCount));

        Info->StatsUpdate.Set([this](const Storage::Models::UpdateStatsInfo& Info) { DownloadDisplay.Update(Info); });
        Info->Begin();
        DownloadBox.show();
        MainStack.child_property_needs_attention(DownloadBox) = true;
    }
}