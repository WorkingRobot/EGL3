#include "Authorization.h"

#include "../storage/models/Authorization.h"
#include "../utils/EmitRAII.h"
#include "../utils/OpenBrowser.h"
#include "../web/epic/auth/DeviceCode.h"
#include "../web/epic/auth/DeviceAuth.h"
#include "../web/epic/auth/TokenToToken.h"

namespace EGL3::Modules {
    AuthorizationModule::AuthorizationModule(Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        PlayButton(Builder.GetWidget<Gtk::Button>("PlayBtn")),
        MenuButton(Builder.GetWidget<Gtk::MenuButton>("PlayDropdown")),
        ButtonState(PlayButtonState::SIGN_IN)
    {
        Dispatcher.connect([this]() { UpdateButton(SignInTask.get() ? PlayButtonState::PLAY : PlayButtonState::SIGN_IN); });

        PlayButton.signal_clicked().connect([this]() { ButtonClick(); });

        auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
        if (!Auth.GetAccountId().empty()) {
            UpdateButton(PlayButtonState::SIGNING_IN);
            SignInTask = std::async(std::launch::async, [this](decltype(Auth)& Auth) {
                Utils::EmitRAII Emitter(Dispatcher);

                Web::Epic::Auth::DeviceAuth FNAuth(AuthorizationSwitch, Auth.GetAccountId(), Auth.GetDeviceId(), Auth.GetSecret());
                if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceAuth::SUCCESS, LogLevel::Warning, "Could not use device auth")) {
                    return false;
                }

                Web::Epic::Auth::TokenToToken LauncherAuth(AuthorizationLauncher, FNAuth.GetOAuthResponse()["access_token"].GetString());
                if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::TokenToToken::SUCCESS, LogLevel::Error, "Could not transfer fn auth to launcher auth")) {
                    return false;
                }

                AuthChanged.emit(
                    AuthClientFN.emplace(FNAuth.GetOAuthResponse(), AuthorizationSwitch),
                    AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), AuthorizationLauncher)
                );

                return true;
            }, Auth);
        }
        else {
            UpdateButton(PlayButtonState::SIGN_IN);
        }
    }

    void AuthorizationModule::UpdateButton(PlayButtonState TargetState) {
        ButtonState = TargetState;
        switch (ButtonState)
        {
        case PlayButtonState::SIGN_IN:
            PlayButton.set_label("Sign In");
            PlayButton.set_sensitive(true);
            MenuButton.set_sensitive(false);
            break;
        case PlayButtonState::SIGNING_IN:
            PlayButton.set_label("Signing In");
            PlayButton.set_sensitive(false);
            MenuButton.set_sensitive(false);
            break;
        case PlayButtonState::PLAY:
            PlayButton.set_label("Play");
            PlayButton.set_sensitive(true);
            MenuButton.set_sensitive(true);
            break;
        case PlayButtonState::PLAYING:
            PlayButton.set_label("Playing");
            PlayButton.set_sensitive(false);
            MenuButton.set_sensitive(false);
            break;
        case PlayButtonState::UPDATE:
            PlayButton.set_label("Update");
            PlayButton.set_sensitive(true);
            MenuButton.set_sensitive(true);
            break;
        case PlayButtonState::UPDATING:
            PlayButton.set_label("Updating");
            PlayButton.set_sensitive(false);
            MenuButton.set_sensitive(false);
            break;
        default:
            PlayButton.set_label("Unknown");
            PlayButton.set_sensitive(false);
            MenuButton.set_sensitive(false);
            break;
        }
    }

    void AuthorizationModule::ButtonClick() {
        switch (ButtonState)
        {
        case PlayButtonState::SIGN_IN:
            UpdateButton(PlayButtonState::SIGNING_IN);
            SignInTask = std::async(std::launch::async, [this]() {
                Utils::EmitRAII Emitter(Dispatcher);

                Web::Epic::Auth::DeviceCode FNAuth(AuthorizationSwitch);
                if (!EGL3_CONDITIONAL_LOG(FNAuth.GetBrowserUrlFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not get browser url")) {
                    return false;
                }
                Utils::OpenInBrowser(FNAuth.GetBrowserUrl());

                if (!EGL3_CONDITIONAL_LOG(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not grab device code")) {
                    return false;
                }

                Web::Epic::Auth::TokenToToken LauncherAuth(AuthorizationLauncher, FNAuth.GetOAuthResponse()["access_token"].GetString());
                if (!EGL3_CONDITIONAL_LOG(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, LogLevel::Error, "Could not transfer fn auth to launcher auth")) {
                    return false;
                }

                AuthChanged.emit(
                    AuthClientFN.emplace(FNAuth.GetOAuthResponse(), AuthorizationSwitch),
                    AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), AuthorizationLauncher)
                );

                auto DevAuthResp = AuthClientFN->CreateDeviceAuth();
                if (!EGL3_CONDITIONAL_LOG(!DevAuthResp.HasError(), LogLevel::Error, "Could not create device auth")) {
                    return false;
                }

                auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
                Auth = Storage::Models::Authorization(DevAuthResp->AccountId, DevAuthResp->DeviceId, DevAuthResp->Secret.value());
                Storage.Flush();

                return true;
            });
            break;
        case PlayButtonState::PLAY:
            printf("PLAY BUTTON\n");
            break;
        case PlayButtonState::UPDATE:
            printf("UPDATE BUTTON\n");
            break;
        }
    }

    const cpr::Authentication AuthorizationModule::AuthorizationLauncher{ "34a02cf8f4414e29b15921876da36f9a", "daafbccc737745039dffe53d94fc76cf" };
    const cpr::Authentication AuthorizationModule::AuthorizationSwitch{ "5229dcd3ac3845208b496649092f251b", "e3bd2d3e-bf8c-4857-9e7d-f3d947d220c7" };
}