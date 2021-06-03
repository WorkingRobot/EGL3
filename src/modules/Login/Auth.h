#pragma once

#include "../../web/epic/EpicClientAuthed.h"
#include "../../web/epic/content/LauncherContentClient.h"
#include "../ModuleList.h"
#include "Chooser.h"
#include "Header.h"
#include "Stack.h"

#include <gtkmm.h>

namespace EGL3::Modules::Login {
    class AuthModule : public BaseModule {
    public:
        AuthModule(ModuleList& Ctx);

        bool IsLoggedIn() const;

        // Will crash if not logged in! Take care.
        Web::Epic::EpicClientAuthed& GetClientFortnite();

        Web::Epic::EpicClientAuthed& GetClientLauncher();

        Web::Epic::Content::LauncherContentClient& GetClientLauncherContent();

        void OpenSignInPage();

        // Returns GetLastError(), 0 if successful
        uint32_t InstallSignInProtocol();

        bool HandleCommandLine(const Glib::RefPtr<Gio::ApplicationCommandLine>& CommandLine);

        void AccountSelected(Storage::Models::AuthUserData Data);

        sigc::signal<void()> LoggedIn;

    private:
        ChooserModule& Chooser;
        HeaderModule& Header;
        StackModule& Stack;
        Storage::Persistent::Store& Storage;
        std::vector<Storage::Models::AuthUserData>& UserData;
        Storage::Models::AuthUserData* SelectedUserData;

        std::future<void> SignInTask;
        Glib::Dispatcher SignInDispatcher;
        Storage::Models::AuthUserData SignInData;
        Glib::Dispatcher LoggedInDispatcher;

        struct ClientData {
            Web::Epic::EpicClientAuthed Fortnite;
            Web::Epic::EpicClientAuthed Launcher;
            Web::Epic::Content::LauncherContentClient Content;
            Utils::Callback<void(const Storage::Models::AuthUserData& NewData)> OnUserDataUpdate;

            ClientData(const rapidjson::Document& FortniteOAuthResp, const rapidjson::Document& LauncherOAuthResp);
            ClientData(const rapidjson::Document& FortniteOAuthResp, Web::Epic::EpicClientAuthed&& Launcher);

            Storage::Models::AuthUserData GetUserData();

        private:
            void SetRefreshCallback();
        };
        std::optional<ClientData> Clients;
    };
}