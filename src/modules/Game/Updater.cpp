#include "Updater.h"

namespace EGL3::Modules::Game {
    UpdaterModule::UpdaterModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage)
    {

    }

    void UpdaterModule::QueueUpdate(Storage::Models::UpdateInfo& UpdateInfo)
    {

    }
}