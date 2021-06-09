#include "Auth.h"

#include "../../utils/formatters/Path.h"
#include "../../utils/Config.h"
#include "../../utils/OpenBrowser.h"
#include "../../utils/Quote.h"
#include "../../utils/UrlEncode.h"
#include "../../web/ClientSecrets.h"
#include "../../web/epic/EpicClientAuthed.h"
#include "../../web/epic/auth/AuthorizationCode.h"
#include "../../web/epic/auth/ExchangeCode.h"
#include "../../web/epic/auth/RefreshToken.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Modules::Login {
    AuthModule::AuthModule(ModuleList& Ctx) :
        Chooser(Ctx.GetModule<ChooserModule>()),
        Header(Ctx.GetModule<HeaderModule>()),
        Stack(Ctx.GetModule<StackModule>()),
        Storage(Ctx.GetStorage()),
        UserData(Storage.Get(Storage::Persistent::Key::Auth).GetUsers())
    {
        Chooser.AccountAddRequest.Set([this]() {
            OpenSignInPage();
        });
        Chooser.AccountClicked.Set([this](Storage::Models::AuthUserData* DataPtr) {
            AccountSelected(*DataPtr);
        });
        Chooser.AccountRemoved.Set([this](Storage::Models::AuthUserData* DataPtr) {
            auto Itr = std::find_if(UserData.begin(), UserData.end(), [DataPtr](const Storage::Models::AuthUserData& Data) {
                return Data.AccountId == DataPtr->AccountId;
            });
            if (Itr != UserData.end()) {
                UserData.erase(Itr);
            }
        });
        Chooser.SetAccounts(UserData.begin(), UserData.end());

        LoggedIn.connect([this]() {
            Clients->OnUserDataUpdate.Set([this](const Storage::Models::AuthUserData& NewData) {
                if (SelectedUserData) {
                    *SelectedUserData = NewData;
                    LoggedInDispatcher.emit();
                    Storage.Flush();
                }
            });

            Storage::Models::AuthUserData NewData = Clients->GetUserData();
            auto Itr = std::find_if(UserData.begin(), UserData.end(), [&NewData](const Storage::Models::AuthUserData& Data) {
                return Data.AccountId == NewData.AccountId;
            });
            if (Itr != UserData.end()) {
                SelectedUserData = &*Itr;
            }
            else {
                SelectedUserData = &UserData.emplace_back();
            }
            *SelectedUserData = NewData;

            LoggedInDispatcher.emit();

            Storage.Flush();
        });

        SignInDispatcher.connect([this]() {
            Stack.DisplayIntermediate(SignInData);
        });

        LoggedInDispatcher.connect([this]() {
            Header.Show(*SelectedUserData);
            Stack.DisplayPrimary();
        });

        Header.Hide();
        Stack.DisplaySignIn();
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

        SignInTask = std::async(std::launch::async, [this, AuthCodeString = AuthCode]() {
            Web::Epic::Auth::AuthorizationCode AuthCode(Web::AuthClientAndroid, AuthCodeString);
            if (!EGL3_ENSURE(AuthCode.GetOAuthResponseFuture().get() == Web::Epic::Auth::AuthorizationCode::SUCCESS, LogLevel::Error, "Could not use auth code")) {
                return;
            }
            Web::Epic::EpicClientAuthed AuthCodeClient(AuthCode.GetOAuthResponse(), Web::AuthClientAndroid);

            auto& AuthData = AuthCodeClient.GetAuthData();
            auto KairosDataResp = AuthCodeClient.GetSettingsForAccounts({ AuthData.AccountId.value() }, { "avatar", "avatarBackground" });
            if (!EGL3_ENSURE(!KairosDataResp.HasError(), LogLevel::Error, "Could not get kairos data from android client")) {
                return;
            }
            
            SignInData = {
                .AccountId = AuthData.AccountId.value(),
                .DisplayName = AuthData.DisplayName.value(),
                .RefreshToken = "",
                .RefreshExpireTime = Web::TimePoint::max()
            };
            for (auto& Setting : KairosDataResp->Values) {
                if (Setting.AccountId != AuthData.AccountId.value()) {
                    continue;
                }
                if (Setting.Key == "avatar") {
                    SignInData.KairosAvatar = Setting.Value;
                }
                else if (Setting.Key == "avatarBackground") {
                    SignInData.KairosBackground = Setting.Value;
                }
            }
            SignInDispatcher.emit();

            auto FortniteCodeResp = AuthCodeClient.GetExchangeCode();
            if (!EGL3_ENSURE(!FortniteCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from android client #1")) {
                return;
            }
            auto LauncherCodeResp = AuthCodeClient.GetExchangeCode();
            if (!EGL3_ENSURE(!LauncherCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from android client #2")) {
                return;
            }

            Web::Epic::Auth::ExchangeCode FortniteAuth(Web::AuthClientPC, FortniteCodeResp->Code);
            if (!EGL3_ENSURE(FortniteAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for fortnite")) {
                return;
            }

            Web::Epic::Auth::ExchangeCode LauncherAuth(Web::AuthClientLauncher, LauncherCodeResp->Code);
            if (!EGL3_ENSURE(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for launcher")) {
                return;
            }

            Clients.emplace(FortniteAuth.GetOAuthResponse(), LauncherAuth.GetOAuthResponse());
            LoggedIn.emit();
        });

        return true;
    }

    void AuthModule::AccountSelected(Storage::Models::AuthUserData Data)
    {
        SignInTask = std::async(std::launch::async, [this, Data]() {
            SignInData = Data;
            SignInDispatcher.emit();

            Web::Epic::Auth::RefreshToken LauncherAuth(Web::AuthClientLauncher, Data.RefreshToken);
            if (!EGL3_ENSURE(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::RefreshToken::SUCCESS, LogLevel::Error, "Could not use refresh token")) {
                return;
            }
            Web::Epic::EpicClientAuthed LauncherClient(LauncherAuth.GetOAuthResponse(), Web::AuthClientLauncher);

            auto FortniteCodeResp = LauncherClient.GetExchangeCode();
            if (!EGL3_ENSURE(!FortniteCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from launcher client")) {
                return;
            }

            Web::Epic::Auth::ExchangeCode FortniteAuth(Web::AuthClientPC, FortniteCodeResp->Code);
            if (!EGL3_ENSURE(FortniteAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for fortnite")) {
                return;
            }

            Clients.emplace(FortniteAuth.GetOAuthResponse(), std::move(LauncherClient));
            LoggedIn.emit();
        });
    }

    AuthModule::ClientData::ClientData(const rapidjson::Document& FortniteOAuthResp, const rapidjson::Document& LauncherOAuthResp) :
        Fortnite(FortniteOAuthResp, Web::AuthClientPC),
        Launcher(LauncherOAuthResp, Web::AuthClientLauncher),
        Content(Launcher, Utils::Config::GetFolder() / "contentcache")
    {
        Launcher.SetKillTokenOnDestruct(false);
        SetRefreshCallback();
    }

    AuthModule::ClientData::ClientData(const rapidjson::Document& FortniteOAuthResp, Web::Epic::EpicClientAuthed&& Launcher) :
        Fortnite(FortniteOAuthResp, Web::AuthClientPC),
        Launcher(std::move(Launcher)),
        Content(this->Launcher, Utils::Config::GetFolder() / "contentcache")
    {
        this->Launcher.SetKillTokenOnDestruct(false);
        SetRefreshCallback();
    }

    Storage::Models::AuthUserData AuthModule::ClientData::GetUserData()
    {
        auto& AuthData = Launcher.GetAuthData();
        Storage::Models::AuthUserData NewData{
            .AccountId = AuthData.AccountId.value(),
            .DisplayName = AuthData.DisplayName.value(),
            .KairosAvatar = "Unknown",
            .KairosBackground = "Unknown",
            .RefreshToken = AuthData.RefreshToken.value(),
            .RefreshExpireTime = AuthData.RefreshExpiresAt.value()
        };
        auto KairosDataResp = Fortnite.GetSettingsForAccounts({ AuthData.AccountId.value() }, { "avatar", "avatarBackground" });
        if (EGL3_ENSURE(!KairosDataResp.HasError(), LogLevel::Error, "Could not get kairos data from fortnite client on launcher refresh")) {
            for (auto& Setting : KairosDataResp->Values) {
                if (Setting.AccountId != AuthData.AccountId.value()) {
                    continue;
                }
                if (Setting.Key == "avatar") {
                    NewData.KairosAvatar = Setting.Value;
                }
                else if (Setting.Key == "avatarBackground") {
                    NewData.KairosBackground = Setting.Value;
                }
            }
        }
        return NewData;
    }

    void AuthModule::ClientData::SetRefreshCallback()
    {
        Launcher.OnRefresh.Set([this](const Web::Epic::Responses::OAuthToken& NewToken) {
            OnUserDataUpdate(GetUserData());
        });
    }
}