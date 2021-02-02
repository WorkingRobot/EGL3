#pragma once

#include "BaseModule.h"

#include "../storage/persistent/Store.h"
#include "../web/epic/EpicClientAuthed.h"
#include "../web/epic/content/LauncherContentClient.h"

#include <future>
#include <sigc++/sigc++.h>

namespace EGL3::Modules {
    class AuthorizationModule : public BaseModule {
    public:
        AuthorizationModule(Storage::Persistent::Store& Storage);

        bool IsLoggedIn() const;

        // Will crash if not logged in! Take care.
        Web::Epic::EpicClientAuthed& GetClientFN();

        Web::Epic::EpicClientAuthed& GetClientLauncher();

        Web::Epic::LauncherContentClient& GetClientLauncherContent();

        void StartLogin();

        // This will not be emitted from the main thread
        sigc::signal<void()> AuthChanged;

    private:
        Storage::Persistent::Store& Storage;

        std::future<void> SignInTask;
        std::optional<Web::Epic::EpicClientAuthed> AuthClientFN;
        std::optional<Web::Epic::EpicClientAuthed> AuthClientLauncher;
        std::optional<Web::Epic::LauncherContentClient> LauncherContentClient;
    };
}