#include "Auth.h"

#include "../../utils/formatters/Path.h"
#include "../../utils/Config.h"
#include "../../utils/OpenBrowser.h"
#include "../../utils/Quote.h"
#include "../../utils/UrlEncode.h"
#include "../../web/ClientSecrets.h"
#include "../../web/epic/EpicClient.h"
#include "../../web/epic/EpicClientAuthed.h"
#include "../Friends/KairosMenu.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Modules::Login {
    AuthModule::AuthModule(ModuleList& Ctx) :
        Stack(Ctx.GetModule<StackModule>()),
        SysTray(Ctx.GetModule<SysTrayModule>()),
        AuthData(Ctx.Get<AuthDataSetting>()),
        SelectedUserData(nullptr)
    {
        LoggedIn.connect([this]() {
            Clients->OnUserDataUpdate.Set([this](const Storage::Models::AuthUserData& NewData) {
                if (SelectedUserData) {
                    *SelectedUserData = NewData;
                    LoggedInDispatcher.emit();
                    AuthData.Flush();
                }
            });

            Storage::Models::AuthUserData NewData = Clients->GetUserData();
            auto& UserData = AuthData->GetUsers();
            auto Itr = std::find_if(UserData.begin(), UserData.end(), [&NewData](const Storage::Models::AuthUserData& Data) {
                return Data.AccountId == NewData.AccountId;
            });
            if (Itr != UserData.end()) {
                SelectedUserData = &*Itr;
            }
            else {
                SelectedUserData = &UserData.emplace_back();
                Itr = UserData.end() - 1;
            }
            *SelectedUserData = NewData;

            LoggedInDispatcher.emit();
            
            AuthData->SetSelectedUser(std::distance(UserData.begin(), Itr));
            AuthData.Flush();
        });

        LoggedOut.connect([this]() {
            SysTray.SetLoggedIn(false);

            AuthData->ClearSelectedUser();
            AuthData.Flush();
        });

        SignInDispatcher.connect([this](const Storage::Models::AuthUserData& Data) { Stack.DisplayIntermediate(Data); });

        LoggedInDispatcher.connect([this]() {
            SysTray.SetLoggedIn(true);
            Stack.DisplayPrimary();
        });

        LoggedInFailureDispatcher.connect([this, &Ctx](const std::string& Message) {
            Stack.DisplaySignIn();
            Clients.reset();
            SelectedUserData = nullptr;

            Ctx.DisplayError(Message, "If this is unexpected, you can check the log files and report the issue to the discord.", "Login Failure", false);
        });

        SysTray.OnLogIn.Set([this]() {
            // Nothing to do, we'll let the user select which account they want.
        });
        SysTray.OnLogOut.Set([this]() {
            LogOut();
        });
        SysTray.OnQuit.Set([this]() {
            return IsLoggedIn() && !LogOutPreflight.emit();
        });
    }

    void AuthModule::StartStartupLogin()
    {
        if (AuthData->IsUserSelected()) {
            AccountSelected(AuthData->GetSelectedUser());
        }
        else {
            Stack.DisplaySignIn();
        }
    }

    bool AuthModule::IsLoggedIn() const
    {
        return Clients.has_value();
    }

    Web::Epic::EpicClientAuthed& AuthModule::GetClientFortnite()
    {
        return Clients->Fortnite;
    }

    Web::Epic::EpicClientAuthed& AuthModule::GetClientLauncher()
    {
        return Clients->Launcher;
    }

    Web::Epic::Content::LauncherContentClient& AuthModule::GetClientLauncherContent()
    {
        return Clients->Content;
    }

    void AuthModule::OpenSignInPage()
    {
        auto Result = InstallSignInProtocol();
        if (EGL3_ENSUREF(Result == ERROR_SUCCESS, LogLevel::Error, "Could not set intent protocol in registry (GLE: {})", Result)) {
            Utils::OpenInBrowser("https://www.epicgames.com/id/login?client_id=3f69e56c7649492c8cc29f1af08a8a12&response_type=code&display=popup&prompt=login");
        }
    }

    struct RegKey {
        RegKey() = default;
        RegKey(const RegKey&) = delete;

        operator HKEY()
        {
            return Key;
        }

        HKEY* operator&()
        {
            return &Key;
        }

        ~RegKey() {
            RegCloseKey(Key);
        }

    private:
        HKEY Key;
    };

    uint32_t AuthModule::InstallSignInProtocol()
    {
        LSTATUS Error;

        // Create main key
        RegKey ProtocolKey;
        Error = RegCreateKeyEx(HKEY_CURRENT_USER, R"(SOFTWARE\Classes\com.epicgames.fortnite)", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &ProtocolKey, NULL);
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        // Set main key's default value
        static constexpr const char ProtocolName[] = "EGL3";
        Error = RegSetValueEx(ProtocolKey, NULL, 0, REG_SZ, (const uint8_t*)ProtocolName, sizeof(ProtocolName));
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        // Add "URL Protocol" as a value
        Error = RegSetValueEx(ProtocolKey, "URL Protocol", 0, REG_SZ, NULL, 0);
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        // Create extra subkeys for launch command
        RegKey CommandKey;
        Error = RegCreateKeyEx(ProtocolKey, R"(shell\open\command)", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &CommandKey, NULL);
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        // Set command subkey's default value to the right command line
        std::string CommandLine = std::format(R"("{}" auth "%1")", Utils::Config::GetExePath());
        Error = RegSetValueEx(CommandKey, NULL, 0, REG_SZ, (const uint8_t*)CommandLine.c_str(), CommandLine.size() + 1);
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        return ERROR_SUCCESS;
    }

    std::string ParseIntentUrl(const char* Input)
    {
        constexpr const char* RedirectUrl = "com.epicgames.fortnite://fnauth/";
        constexpr const char* SuccessQuery = "code=";
        if (strncmp(RedirectUrl, Input, strlen(RedirectUrl)) != 0) {
            return "";
        }
        const char* Query = Input + strlen(RedirectUrl);
        if (strlen(Query) == 0 || *Query != '?') {
            return "";
        }
        ++Query;
        // No need to actually parse it like a normal query string, just look at the first parameter
        if (strncmp(SuccessQuery, Query, strlen(SuccessQuery)) != 0) {
            return "";
        }
        Query += strlen(SuccessQuery);
        if (strchr(Query, '&') != 0) {
            return "";
        }
        return Utils::UrlDecode(Query);
    }

    bool AuthModule::HandleCommandLine(const Glib::RefPtr<Gio::ApplicationCommandLine>& CommandLine)
    {
        int argc;
        char** argv = CommandLine->get_arguments(argc);
        if (argc != 3) {
            return false;
        }
        if (strcmp(argv[1], "auth") != 0) {
            return false;
        }

        auto AuthCode = ParseIntentUrl(argv[2]);
        if (AuthCode.empty()) {
            return true;
        }

        if (IsLoggedIn() && !LogOut(false)) {
            return true;
        }

        SignInTask = std::async(std::launch::async, [this, AuthCodeString = AuthCode]() {
            Web::Epic::EpicClient AuthClient;

            auto AuthDataResp = AuthClient.AuthorizationCode(Web::AuthClientAndroid, AuthCodeString);
            if (!EGL3_ENSURE(!AuthDataResp.HasError(), LogLevel::Error, "Could not use auth code")) {
                OnLogInFailure("Could not login with the given credentials");
                return;
            }
            Web::Epic::EpicClientAuthed AuthCodeClient(AuthDataResp.Get(), Web::AuthClientAndroid);
            
            SignInDispatcher.emit(Storage::Models::AuthUserData{
                .AccountId = AuthDataResp->AccountId.value(),
                .DisplayName = AuthDataResp->DisplayName.value(),
                .RefreshExpireTime = Web::TimePoint::max()
            });

            auto FortniteCodeResp = AuthCodeClient.GetExchangeCode();
            if (!EGL3_ENSURE(!FortniteCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from android client #1")) {
                OnLogInFailure("Could not login with the given credentials");
                return;
            }
            auto LauncherCodeResp = AuthCodeClient.GetExchangeCode();
            if (!EGL3_ENSURE(!LauncherCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from android client #2")) {
                OnLogInFailure("Could not login with the given credentials");
                return;
            }

            auto FortniteAuthResp = AuthClient.ExchangeCode(Web::AuthClientPC, FortniteCodeResp->Code);
            if (!EGL3_ENSURE(!FortniteAuthResp.HasError(), LogLevel::Error, "Could not use exchange code for fortnite")) {
                OnLogInFailure("Could not login with the given credentials");
                return;
            }

            auto LauncherAuthResp = AuthClient.ExchangeCode(Web::AuthClientLauncher, LauncherCodeResp->Code);
            if (!EGL3_ENSURE(!LauncherAuthResp.HasError(), LogLevel::Error, "Could not use exchange code for launcher")) {
                OnLogInFailure("Could not login with the given credentials");
                return;
            }

            Clients.emplace(FortniteAuthResp.Get(), LauncherAuthResp.Get());
            LoggedIn.emit();
        });

        return true;
    }

    void AuthModule::AccountSelected(Storage::Models::AuthUserData Data)
    {
        if (IsLoggedIn() && !LogOut(false)) {
            return;
        }

        SignInTask = std::async(std::launch::async, [this, Data]() {
            Web::Epic::EpicClient AuthClient;

            SignInDispatcher.emit(Data);

            auto LauncherAuthResp = AuthClient.RefreshToken(Web::AuthClientLauncher, Data.RefreshToken);
            if (!EGL3_ENSURE(!LauncherAuthResp.HasError(), LogLevel::Error, "Could not use refresh token")) {
                OnLogInFailure("Could not login with the selected account");
                return;
            }
            Web::Epic::EpicClientAuthed LauncherClient(LauncherAuthResp.Get(), Web::AuthClientLauncher);

            auto FortniteCodeResp = LauncherClient.GetExchangeCode();
            if (!EGL3_ENSURE(!FortniteCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from launcher client")) {
                OnLogInFailure("Could not login with the selected account");
                return;
            }

            auto FortniteAuthResp = AuthClient.ExchangeCode(Web::AuthClientPC, FortniteCodeResp->Code);
            if (!EGL3_ENSURE(!FortniteAuthResp.HasError(), LogLevel::Error, "Could not use exchange code for fortnite")) {
                OnLogInFailure("Could not login with the selected account");
                return;
            }

            Clients.emplace(FortniteAuthResp.Get(), std::move(LauncherClient));
            LoggedIn.emit();
        });
    }

    void AuthModule::AccountSelectedEGL()
    {
        if (IsLoggedIn() && !LogOut(false)) {
            return;
        }

        SignInTask = std::async(std::launch::async, [this]() {
            Web::Epic::EpicClient AuthClient;
            
            auto LauncherAuthResp = AuthClient.RefreshToken(Web::AuthClientLauncher, RememberMe.GetProfile()->Token);
            if (!EGL3_ENSURE(!LauncherAuthResp.HasError(), LogLevel::Error, "Could not use EGL refresh token")) {
                OnLogInFailure("Could not use EGL's credentials. You'll need to login manually with the \"Add New Account\" button");
                return;
            }
            Web::Epic::EpicClientAuthed LauncherClient(LauncherAuthResp.Get(), Web::AuthClientLauncher);

            auto& AuthData = LauncherClient.GetAuthData();
            SignInDispatcher.emit(Storage::Models::AuthUserData{
                .AccountId = AuthData.AccountId.value(),
                .DisplayName = AuthData.DisplayName.value(),
                .KairosAvatar = "",
                .KairosBackground = "",
                .RefreshToken = "",
                .RefreshExpireTime = Web::TimePoint::max()
            });

            auto FortniteCodeResp = LauncherClient.GetExchangeCode();
            if (!EGL3_ENSURE(!FortniteCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from launcher client")) {
                OnLogInFailure("Could not use EGL's credentials. You'll need to login manually with the \"Add New Account\" button");
                return;
            }

            auto FortniteAuthResp = AuthClient.ExchangeCode(Web::AuthClientPC, FortniteCodeResp->Code);
            if (!EGL3_ENSURE(!FortniteAuthResp.HasError(), LogLevel::Error, "Could not use exchange code for fortnite")) {
                OnLogInFailure("Could not use EGL's credentials. You'll need to login manually with the \"Add New Account\" button");
                return;
            }

            Clients.emplace(FortniteAuthResp.Get(), std::move(LauncherClient));
            LoggedIn.emit();
        });
    }

    bool AuthModule::AccountRemoved(const std::string& AccountId)
    {
        auto& UserData = AuthData->GetUsers();
        auto Itr = std::find_if(UserData.begin(), UserData.end(), [&](const Storage::Models::AuthUserData& Data) {
            return Data.AccountId == AccountId;
        });
        if (Itr == UserData.end() || &*Itr == SelectedUserData) {
            return false;
        }
        UserData.erase(Itr);
        return true;
    }

    bool AuthModule::LogOut(bool DisplaySignIn)
    {
        if (!LogOutPreflight.emit()) {
            return false;
        }

        if (DisplaySignIn) {
            Stack.DisplaySignIn();
        }
        LoggedOut.emit();
        Clients.reset();
        SelectedUserData = nullptr;

        return true;
    }

    std::deque<Storage::Models::AuthUserData>& AuthModule::GetUserData()
    {
        return AuthData->GetUsers();
    }

    Storage::Models::AuthUserData* AuthModule::GetSelectedUserData()
    {
        return SelectedUserData;
    }

    Utils::EGL::RememberMe& AuthModule::GetRememberMe()
    {
        return RememberMe;
    }

    void AuthModule::OnLogInFailure(const std::string& Message)
    {
        LoggedInFailureDispatcher.emit(Message);
    }

    AuthModule::ClientData::ClientData(const Web::Epic::Responses::OAuthToken& FortniteAuthData, const Web::Epic::Responses::OAuthToken& LauncherAuthData) :
        Fortnite(FortniteAuthData, Web::AuthClientPC),
        Launcher(LauncherAuthData, Web::AuthClientLauncher),
        Content(Launcher, Utils::Config::GetFolder() / "contentcache")
    {
        Launcher.SetKillTokenOnDestruct(false);
        SetRefreshCallback();
    }

    AuthModule::ClientData::ClientData(const Web::Epic::Responses::OAuthToken& FortniteAuthData, Web::Epic::EpicClientAuthed&& Launcher) :
        Fortnite(FortniteAuthData, Web::AuthClientPC),
        Launcher(std::move(Launcher)),
        Content(this->Launcher, Utils::Config::GetFolder() / "contentcache")
    {
        this->Launcher.SetKillTokenOnDestruct(false);
        SetRefreshCallback();
    }

    Storage::Models::AuthUserData AuthModule::ClientData::GetUserData()
    {
        auto& AuthData = Launcher.GetAuthData();
        return {
            .AccountId = AuthData.AccountId.value(),
            .DisplayName = AuthData.DisplayName.value(),
            .KairosAvatar = Friends::KairosMenuModule::GetRandomKairosAvatar(),
            .KairosBackground = Friends::KairosMenuModule::GetRandomKairosBackground(),
            .RefreshToken = AuthData.RefreshToken.value(),
            .RefreshExpireTime = AuthData.RefreshExpiresAt.value()
        };
    }

    void AuthModule::ClientData::SetRefreshCallback()
    {
        Launcher.OnRefresh.Set([this](const Web::Epic::Responses::OAuthToken& NewToken) {
            OnUserDataUpdate(GetUserData());
        });
    }
}