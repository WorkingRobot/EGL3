#pragma once

#include "../storage/persistent/Store.h"
#include "../web/epic/EpicClientAuthed.h"
#include "../web/epic/content/LauncherContentClient.h"
#include "ModuleList.h"

#include <future>

namespace EGL3::Modules {
    class AuthorizationModule : public BaseModule {
    public:
        AuthorizationModule(ModuleList& Ctx);

        bool IsLoggedIn() const;

        // Will crash if not logged in! Take care.
        Web::Epic::EpicClientAuthed& GetClientFN();

        Web::Epic::EpicClientAuthed& GetClientLauncher();

        Web::Epic::Content::LauncherContentClient& GetClientLauncherContent();

        bool StartLoginStored();

        void StartLogin();

        // This will not be emitted from the main thread
        sigc::signal<void(bool LoggedIn)> AuthChanged;

    private:
        Storage::Persistent::Store& Storage;

        std::future<void> SignInTask;
        std::optional<Web::Epic::EpicClientAuthed> AuthClientFN;
        std::optional<Web::Epic::EpicClientAuthed> AuthClientLauncher;
        std::optional<Web::Epic::Content::LauncherContentClient> LauncherContentClient;
    };
}