#include "Authorization.h"

#include "../storage/models/Authorization.h"
#include "../utils/Config.h"
#include "../utils/OpenBrowser.h"
#include "../web/epic/auth/DeviceAuth.h"
#include "../web/epic/auth/DeviceCode.h"
#include "../web/epic/auth/ExchangeCode.h"
#include "../web/ClientSecrets.h"

namespace EGL3::Modules {
    AuthorizationModule::AuthorizationModule(Storage::Persistent::Store& Storage) :
        Storage(Storage)
    {
        
    }

    bool AuthorizationModule::IsLoggedIn() const
    {
        return AuthClientFN.has_value() && AuthClientLauncher.has_value();
    }

    Web::Epic::EpicClientAuthed& AuthorizationModule::GetClientFN()
    {
        EGL3_CONDITIONAL_LOG(AuthClientFN.has_value(), LogLevel::Critical, "Expected to be logged in. No FN client found.");
        return AuthClientFN.value();
    }

    Web::Epic::EpicClientAuthed& AuthorizationModule::GetClientLauncher()
    {
        EGL3_CONDITIONAL_LOG(AuthClientLauncher.has_value(), LogLevel::Critical, "Expected to be logged in. No launcher client found.");
        return AuthClientLauncher.value();
    }

    Web::Epic::Content::LauncherContentClient& AuthorizationModule::GetClientLauncherContent()
    {
        EGL3_CONDITIONAL_LOG(LauncherContentClient.has_value(), LogLevel::Critical, "Expected to be logged in. No launcher content client found.");
        return LauncherContentClient.value();
    }

    bool AuthorizationModule::StartLoginStored()
    {
        auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
        if (!Auth.GetAccountId().empty()) {
            SignInTask = std::async(std::launch::async, [this](std::reference_wrapper<const Storage::Models::Authorization> Auth) {
                Web::Epic::Auth::DeviceAuth FNAuth(Web::AuthClientSwitch, Auth.get().GetAccountId(), Auth.get().GetDeviceId(), Auth.get().GetSecret());
                if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceAuth::SUCCESS, LogLevel::Warning, "Could not use device auth")) {
                    AuthChanged.emit(false);
                    return;
                }

                AuthClientFN.emplace(FNAuth.GetOAuthResponse(), Web::AuthClientSwitch);
                auto ExchCodeResp = AuthClientFN->GetExchangeCode();
                if (!EGL3_CONDITIONAL_LOG(!ExchCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from fn client")) {
                    AuthChanged.emit(false);
                    return;
                }

                Web::Epic::Auth::ExchangeCode LauncherAuth(Web::AuthClientLauncher, ExchCodeResp->Code);
                if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for launcher auth")) {
                    AuthChanged.emit(false);
                    return;
                }

                AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), Web::AuthClientLauncher);
                LauncherContentClient.emplace(AuthClientLauncher.value(), Utils::Config::GetFolder() / "contentcache");
                LauncherContentClient->LoadSdMetaData();
                AuthChanged.emit(true);
            }, std::cref(Auth));
            return true;
        }
        return false;
    }

    void AuthorizationModule::StartLogin() {
        if (IsLoggedIn()) {
            EGL3_LOG(LogLevel::Warning, "Tried to log in while already logged in");
            AuthChanged.emit(true);
            return;
        }

        SignInTask = std::async(std::launch::async, [this]() {
            Web::Epic::Auth::DeviceCode FNAuth(Web::AuthClientSwitch);
            if (!EGL3_CONDITIONAL_LOG(FNAuth.GetBrowserUrlFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not get browser url")) {
                AuthChanged.emit(false);
                return;
            }
            Utils::OpenInBrowser(FNAuth.GetBrowserUrl());

            if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not grab device code")) {
                AuthChanged.emit(false);
                return;
            }

            AuthClientFN.emplace(FNAuth.GetOAuthResponse(), Web::AuthClientSwitch);
            auto ExchCodeResp = AuthClientFN->GetExchangeCode();
            if (!EGL3_CONDITIONAL_LOG(!ExchCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from fn client")) {
                AuthChanged.emit(false);
                return;
            }

            Web::Epic::Auth::ExchangeCode LauncherAuth(Web::AuthClientLauncher, ExchCodeResp->Code);
            if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for launcher auth")) {
                AuthChanged.emit(false);
                return;
            }

            AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), Web::AuthClientLauncher);
            LauncherContentClient.emplace(AuthClientLauncher.value());
            AuthChanged.emit(true);

            auto DevAuthResp = AuthClientFN->CreateDeviceAuth();
            if (!EGL3_CONDITIONAL_LOG(!DevAuthResp.HasError(), LogLevel::Error, "Could not create device auth, you'll need to log in again next time")) {
                return;
            }

            auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
            Auth = Storage::Models::Authorization(DevAuthResp->AccountId, DevAuthResp->DeviceId, DevAuthResp->Secret.value());
            Storage.Flush();
        });
    }
}