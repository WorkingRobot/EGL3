#include "Authorization.h"

#include "../storage/models/Authorization.h"
#include "../utils/OpenBrowser.h"
#include "../web/epic/auth/DeviceAuth.h"
#include "../web/epic/auth/DeviceCode.h"
#include "../web/epic/auth/ExchangeCode.h"

namespace EGL3::Modules {
    AuthorizationModule::AuthorizationModule(Storage::Persistent::Store& Storage) :
        Storage(Storage)
    {
        auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
        if (!Auth.GetAccountId().empty()) {
            SignInTask = std::async(std::launch::async, [this](std::reference_wrapper<const Storage::Models::Authorization> Auth) {
                Web::Epic::Auth::DeviceAuth FNAuth(AuthorizationSwitch, Auth.get().GetAccountId(), Auth.get().GetDeviceId(), Auth.get().GetSecret());
                if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceAuth::SUCCESS, LogLevel::Warning, "Could not use device auth")) {
                    return;
                }

                AuthClientFN.emplace(FNAuth.GetOAuthResponse(), AuthorizationSwitch);
                auto ExchCodeResp = AuthClientFN->GetExchangeCode();
                if (!EGL3_CONDITIONAL_LOG(!ExchCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from fn client")) {
                    return;
                }

                Web::Epic::Auth::ExchangeCode LauncherAuth(AuthorizationLauncher, ExchCodeResp->Code);
                if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for launcher auth")) {
                    return;
                }

                AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), AuthorizationLauncher);
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

    void AuthorizationModule::StartLogin() {
        if (SignInTask.valid() || IsLoggedIn()) {
            EGL3_LOG(LogLevel::Warning, "Tried to log in while already trying to or already logged in");
            return;
        }

        SignInTask = std::async(std::launch::async, [this]() {
            Web::Epic::Auth::DeviceCode FNAuth(AuthorizationSwitch);
            if (!EGL3_CONDITIONAL_LOG(FNAuth.GetBrowserUrlFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not get browser url")) {
                return;
            }
            Utils::OpenInBrowser(FNAuth.GetBrowserUrl());

            if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not grab device code")) {
                return;
            }

            AuthClientFN.emplace(FNAuth.GetOAuthResponse(), AuthorizationSwitch);
            auto ExchCodeResp = AuthClientFN->GetExchangeCode();
            if (!EGL3_CONDITIONAL_LOG(!ExchCodeResp.HasError(), LogLevel::Error, "Could not get exchange code from fn client")) {
                return;
            }

            Web::Epic::Auth::ExchangeCode LauncherAuth(AuthorizationLauncher, ExchCodeResp->Code);
            if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ExchangeCode::SUCCESS, LogLevel::Error, "Could not use exchange code for launcher auth")) {
                return;
            }

            AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), AuthorizationLauncher);
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

    const cpr::Authentication AuthorizationModule::AuthorizationLauncher{ "34a02cf8f4414e29b15921876da36f9a", "daafbccc737745039dffe53d94fc76cf" };
    const cpr::Authentication AuthorizationModule::AuthorizationSwitch{ "5229dcd3ac3845208b496649092f251b", "e3bd2d3e-bf8c-4857-9e7d-f3d947d220c7" };
}