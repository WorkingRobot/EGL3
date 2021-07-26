#pragma once

#include "../../storage/models/Authorization.h"
#include "../../utils/DataDispatcher.h"
#include "../../web/epic/EpicClientAuthed.h"
#include "../../web/epic/content/LauncherContentClient.h"
#include "../../utils/egl/RememberMe.h"
#include "../ModuleList.h"
#include "../SysTray.h"
#include "Stack.h"

#include <gtkmm.h>

namespace EGL3::Modules::Login {
    class AuthModule : public BaseModule {
        struct PreflightAccumulator
        {
            using result_type = bool;

            template<typename Itr>
            bool operator()(Itr Begin, Itr End) const {
                for (; Begin != End; ++Begin) {
                    if (!*Begin) {
                        return false;
                    }
                }
                return true;
            }
        };

    public:
        AuthModule(ModuleList& Ctx);

        void StartStartupLogin();

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

        void AccountSelectedEGL();

        bool AccountRemoved(const std::string& AccountId);

        bool LogOut(bool DisplaySignIn = true);

        std::deque<Storage::Models::AuthUserData>& GetUserData();

        Storage::Models::AuthUserData* GetSelectedUserData();

        Utils::EGL::RememberMe& GetRememberMe();

        sigc::signal<void()> LoggedIn;

        sigc::signal0<bool, PreflightAccumulator> LogOutPreflight;

        sigc::signal<void()> LoggedOut;

    private:
        void OnLogInFailure(const std::string& Message);

        StackModule& Stack;
        SysTrayModule& SysTray;

        using AuthDataSetting = Storage::Persistent::Setting<Utils::Crc32("Auth"), Storage::Models::Authorization>;
        Storage::Persistent::SettingHolder<AuthDataSetting> AuthData;
        Storage::Models::AuthUserData* SelectedUserData;

        std::future<void> SignInTask;
        Utils::DataDispatcher<Storage::Models::AuthUserData> SignInDispatcher;
        Glib::Dispatcher LoggedInDispatcher;

        Utils::DataDispatcher<std::string> LoggedInFailureDispatcher;

        Utils::EGL::RememberMe RememberMe;

        struct ClientData {
            Web::Epic::EpicClientAuthed Fortnite;
            Web::Epic::EpicClientAuthed Launcher;
            Web::Epic::Content::LauncherContentClient Content;
            Utils::Callback<void(const Storage::Models::AuthUserData& NewData)> OnUserDataUpdate;

            ClientData(const Web::Epic::Responses::OAuthToken& FortniteAuthData, const Web::Epic::Responses::OAuthToken& LauncherAuthData);
            ClientData(const Web::Epic::Responses::OAuthToken& FortniteAuthData, Web::Epic::EpicClientAuthed&& Launcher);

            Storage::Models::AuthUserData GetUserData();

        private:
            void SetRefreshCallback();
        };
        std::optional<ClientData> Clients;
    };
}