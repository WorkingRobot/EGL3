#include "Authorization.h"

#include "../storage/models/Authorization.h"
#include "../utils/OpenBrowser.h"
#include "../web/epic/auth/DeviceAuth.h"
#include "../web/epic/auth/DeviceCode.h"
#include "../web/epic/auth/ExchangeCode.h"
#include "../web/ClientSecrets.h"

namespace EGL3::Modules {
    AuthorizationModule::AuthorizationModule(Storage::Persistent::Store& Storage) :
        Storage(Storage)
    {
        auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
        if (!Auth.GetAccountId().empty()) {
            SignInTask = std::async(std::launch::async, [this](std::reference_wrapper<const Storage::Models::Authorization> Auth) {
                Web::Epic::Auth::DeviceAuth FNAuth(Web::AuthClientSwitch, Auth.get().GetAccountId(), Auth.get().GetDeviceId(), Auth.get().GetSecret());
                if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceAuth::SUCCESS, LogLevel::Warning, "Could not use device auth")) {
                    return;
                }

                AuthClientFN.emplace(FNAuth.GetOAuthResponse(), Web::AuthClientSwitch);
                auto ExchCodeResp = AuthClientFN->GetExchangeCode();
                if (!EGL3_CONDITIONAL_LOG(!ExchCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from fn client")) {
                    return;
                }

                Web::Epic::Auth::ExchangeCode LauncherAuth(Web::AuthClientLauncher, ExchCodeResp->Code);
                if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for launcher auth")) {
                    return;
                }

                AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), Web::AuthClientLauncher);
                LauncherContentClient.emplace(AuthClientLauncher.value());
                AuthChanged.emit();
            }, std::cref(Auth));
        }
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

    void AuthorizationModule::StartLogin() {
        if (IsLoggedIn()) {
            EGL3_LOG(LogLevel::Warning, "Tried to log in while already logged in");
            AuthChanged.emit();
            return;
        }

        SignInTask = std::async(std::launch::async, [this]() {
            Web::Epic::Auth::DeviceCode FNAuth(Web::AuthClientSwitch);
            if (!EGL3_CONDITIONAL_LOG(FNAuth.GetBrowserUrlFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not get browser url")) {
                return;
            }
            Utils::OpenInBrowser(FNAuth.GetBrowserUrl());

            if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not grab device code")) {
                return;
            }

            AuthClientFN.emplace(FNAuth.GetOAuthResponse(), Web::AuthClientSwitch);
            auto ExchCodeResp = AuthClientFN->GetExchangeCode();
            if (!EGL3_CONDITIONAL_LOG(!ExchCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from fn client")) {
                return;
            }

            Web::Epic::Auth::ExchangeCode LauncherAuth(Web::AuthClientLauncher, ExchCodeResp->Code);
            if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for launcher auth")) {
                return;
            }

            AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), Web::AuthClientLauncher);
            LauncherContentClient.emplace(AuthClientLauncher.value());
            AuthChanged.emit();

            auto DevAuthResp = AuthClientFN->CreateDeviceAuth();
            if (!EGL3_CONDITIONAL_LOG(!DevAuthResp.HasError(), LogLevel::Error, "Could not create device auth")) {
                return;
            }

            auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
            Auth = Storage::Models::Authorization(DevAuthResp->AccountId, DevAuthResp->DeviceId, DevAuthResp->Secret.value());
            Storage.Flush();
        });
    }
}