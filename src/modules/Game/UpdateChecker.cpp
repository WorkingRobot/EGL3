#include "UpdateChecker.h"

namespace EGL3::Modules::Game {
    UpdateCheckerModule::UpdateCheckerModule(ModuleList& Modules, Storage::Persistent::Store& Storage) :
        RequestVersionNum([]() { return 0; }),
        Storage(Storage),
        Auth(Modules.GetModule<AuthorizationModule>())
    {
        auto& Freq = Storage.Get(Storage::Persistent::Key::UpdateFrequency);
        if (Freq < std::chrono::seconds(10)) {
            Freq = std::chrono::seconds(10);
        }

        Auth.AuthChanged.connect([this]() {
            if (!Future.valid()) {
                Future = std::async(std::launch::async, &UpdateCheckerModule::BackgroundTask, this);
            }
        });
    }

    UpdateCheckerModule::~UpdateCheckerModule()
    {
        {
            std::unique_lock Lock(Mutex);
            Cancelled = true;
        }
        CV.notify_all();
        if (Future.valid()) {
            Future.get();
        }
    }

    void UpdateCheckerModule::CheckForUpdate()
    {
        if (Auth.IsLoggedIn()) {
            uint32_t CurrentVersion = RequestVersionNum();
            if (CurrentVersion == 0) { // Currently updating, etc.
                return;
            }
            auto Resp = Auth.GetClientLauncher().GetDownloadInfo("Windows", "Live", "4fe75bbc5a674f4f9b356b5c90567da5", "Fortnite");
            if (!Resp.HasError()) {
                //Resp->Elements[0].Manifests[0].
                // Todo: make manifests element get a "GetUrl()" function or something similar
            }
        }
    }

    void UpdateCheckerModule::BackgroundTask()
    {
        std::unique_lock Lock(Mutex);
        do {
            CheckForUpdate();
        } while (!CV.wait_for(Lock, Storage.Get(Storage::Persistent::Key::UpdateFrequency), [this]() { return Cancelled; }));
    }
}