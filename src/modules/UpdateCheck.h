#pragma once

#include "../utils/Callback.h"
#include "../web/egl3/EGL3Client.h"
#include "Login/Auth.h"
#include "ModuleList.h"
#include "SysTray.h"

namespace EGL3::Modules {
    class UpdateCheckModule : public BaseModule {
    public:
        UpdateCheckModule(ModuleList& Ctx);

        ~UpdateCheckModule();

    private:
        void DisplayUpdate(const Web::EGL3::Responses::VersionInfo& VersionData);

        void OnUpdateAvailable(const Web::EGL3::Responses::VersionInfo& VersionData);

        void CheckForUpdate();

        void BackgroundTask();

        Gtk::Dialog& Dialog;
        Gtk::Label& DialogPatchNotes;
        Gtk::Label& DialogDate;
        Gtk::Label& DialogVersion;
        Gtk::Label& DialogVersionHR;
        Gtk::Button& DialogAccept;
        Gtk::Button& DialogReject;

        Login::AuthModule& Auth;
        SysTrayModule& SysTray;

        Web::EGL3::EGL3Client Client;

        int64_t LatestVersionNum;

        std::mutex Mutex;
        std::condition_variable CV;
        bool Cancelled;
        std::future<void> Future;
    };
}