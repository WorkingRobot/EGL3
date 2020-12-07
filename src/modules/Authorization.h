#pragma once

#include "BaseModule.h"

#include <gtkmm.h>

#include "../storage/models/Authorization.h"
#include "../storage/persistent/Store.h"
#include "../utils/GladeBuilder.h"
#include "../utils/OpenBrowser.h"
#include "../web/epic/EpicClientAuthed.h"
#include "../web/epic/auth/DeviceCode.h"
#include "../web/epic/auth/DeviceAuth.h"
#include "../web/epic/auth/TokenToToken.h"

namespace EGL3::Modules {
	class AuthorizationModule : public BaseModule {
	public:
		AuthorizationModule(Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
			Storage(Storage),
			PlayButton(Builder.GetWidget<Gtk::Button>("PlayBtn")),
			MenuButton(Builder.GetWidget<Gtk::MenuButton>("PlayDropdown"))
		{
			Dispatcher.connect([this]() { UpdateButton(SignInTask.get() ? PlayButtonState::PLAY : PlayButtonState::SIGN_IN); });

			PlayButton.signal_clicked().connect([this]() { ButtonClick(); });

			auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
			if (!Auth.AccountId.empty()) {
				UpdateButton(PlayButtonState::SIGNING_IN);
				SignInTask = std::async(std::launch::async, [this](decltype(Auth)& Auth) {
					Web::Epic::Auth::DeviceAuth FNAuth(AuthorizationSwitch, Auth.AccountId, Auth.DeviceId, Auth.Secret);
					EGL3_ASSERT(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceAuth::SUCCESS, "Could not use device auth");

					Web::Epic::Auth::TokenToToken LauncherAuth(AuthorizationLauncher, FNAuth.GetOAuthResponse()["access_token"].GetString());
					EGL3_ASSERT(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::TokenToToken::SUCCESS, "Could not transfer fn auth to launcher auth");

					AuthChanged.emit(
						AuthClientFN.emplace(FNAuth.GetOAuthResponse(), AuthorizationSwitch),
						AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), AuthorizationLauncher)
					);

					Dispatcher.emit();
					return true;
					}, Auth);
			}
			else {
				UpdateButton(PlayButtonState::SIGN_IN);
			}
		}

		~AuthorizationModule() {
			
		}

		enum class PlayButtonState {
			SIGN_IN,
			SIGNING_IN,
			PLAY,
			PLAYING,
			UPDATE,
			UPDATING
		};

		// Update the actual button state, called from the dispatcher
		void UpdateButton(PlayButtonState TargetState) {
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

		// This will not be emitted from the main thread
		sigc::signal<void(Web::Epic::EpicClientAuthed&, Web::Epic::EpicClientAuthed&)> AuthChanged;

	private:
		PlayButtonState ButtonState = PlayButtonState::SIGN_IN; // Default state, doesn't matter too much

		void ButtonClick() {
			switch (ButtonState)
			{
			case PlayButtonState::SIGN_IN:
				UpdateButton(PlayButtonState::SIGNING_IN);
				SignInTask = std::async(std::launch::async, [this]() {
					Web::Epic::Auth::DeviceCode FNAuth(AuthorizationSwitch);
					EGL3_ASSERT(FNAuth.GetBrowserUrlFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, "Could not get browser url");
					Utils::OpenInBrowser(FNAuth.GetBrowserUrl());

					EGL3_ASSERT(FNAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::DeviceCode::SUCCESS, "Could not grab device code");

					Web::Epic::Auth::TokenToToken LauncherAuth(AuthorizationLauncher, FNAuth.GetOAuthResponse()["access_token"].GetString());
					EGL3_ASSERT(LauncherAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::TokenToToken::SUCCESS, "Could not transfer fn auth to launcher auth");

					AuthChanged.emit(
						AuthClientFN.emplace(FNAuth.GetOAuthResponse(), AuthorizationSwitch),
						AuthClientLauncher.emplace(LauncherAuth.GetOAuthResponse(), AuthorizationLauncher)
					);

					auto DevAuthResp = AuthClientFN->CreateDeviceAuth();
					EGL3_ASSERT(!DevAuthResp.HasError(), "Could not create device auth");

					auto& Auth = Storage.Get(Storage::Persistent::Key::Auth);
					Auth.AccountId = DevAuthResp->AccountId;
					Auth.DeviceId = DevAuthResp->DeviceId;
					Auth.Secret = DevAuthResp->Secret.value();
					Storage.Flush();

					Dispatcher.emit();
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

		static inline const cpr::Authentication AuthorizationLauncher{ "34a02cf8f4414e29b15921876da36f9a", "daafbccc737745039dffe53d94fc76cf" };
		static inline const cpr::Authentication AuthorizationSwitch{ "5229dcd3ac3845208b496649092f251b", "e3bd2d3e-bf8c-4857-9e7d-f3d947d220c7" };

		Storage::Persistent::Store& Storage;
		Gtk::Button& PlayButton;
		Gtk::MenuButton& MenuButton;

		std::future<bool> SignInTask;
		Glib::Dispatcher Dispatcher;
		std::optional<Web::Epic::EpicClientAuthed> AuthClientFN;
		std::optional<Web::Epic::EpicClientAuthed> AuthClientLauncher;
	};
}