#pragma once

#include "BaseModule.h"

#include "../storage/persistent/Store.h"
#include "../utils/GladeBuilder.h"
#include "../web/epic/EpicClientAuthed.h"

#include <future>
#include <gtkmm.h>

namespace EGL3::Modules {
	class AuthorizationModule : public BaseModule {
	public:
		AuthorizationModule(Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

		enum class PlayButtonState {
			SIGN_IN,
			SIGNING_IN,
			PLAY,
			PLAYING,
			UPDATE,
			UPDATING
		};

		// Update the actual button state, called from the dispatcher
		void UpdateButton(PlayButtonState TargetState);

		// This will not be emitted from the main thread
		sigc::signal<void(Web::Epic::EpicClientAuthed&, Web::Epic::EpicClientAuthed&)> AuthChanged;

	private:
		PlayButtonState ButtonState; // Default state, doesn't matter too much

		void ButtonClick();

		static const cpr::Authentication AuthorizationLauncher;
		static const cpr::Authentication AuthorizationSwitch;

		Storage::Persistent::Store& Storage;
		Gtk::Button& PlayButton;
		Gtk::MenuButton& MenuButton;

		std::future<bool> SignInTask;
		Glib::Dispatcher Dispatcher;
		std::optional<Web::Epic::EpicClientAuthed> AuthClientFN;
		std::optional<Web::Epic::EpicClientAuthed> AuthClientLauncher;
	};
}