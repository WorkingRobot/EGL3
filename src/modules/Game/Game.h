#pragma once

#include "../../storage/persistent/Store.h"
#include "../../utils/GladeBuilder.h"
#include "../../widgets/InstallLocationDialog.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../Authorization.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class GameModule : public BaseModule {
    public:
        GameModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

        enum class State {
            SignIn,
            Play,
            Update,
            Install,

            SigningIn,
            Playing,
            Updating,
            Installing,
            Uninstalling
        };

    private:
        void OnAuthChanged();

        void UpdateToState(const char* NewLabel, bool Playable = false, bool Menuable = false);

        void UpdateToCurrentState();

        void PlayClicked();

        void CreateInstall(const std::string& Filename);

        const Storage::Models::GameInstall* GetInstall() const;

        Storage::Persistent::Store& Storage;
        AsyncFFModule& AsyncFF;
        AuthorizationModule& Auth;

        Gtk::Button& PlayBtn;
        Gtk::MenuButton& PlayMenuBtn;
        Gtk::MenuItem& PlayMenuVerifyOpt;
        Gtk::MenuItem& PlayMenuModifyOpt;
        Gtk::MenuItem& PlayMenuSignOutOpt;

        Glib::Dispatcher CurrentStateDispatcher;
        State CurrentState;

        Widgets::InstallLocationDialog InstallDialog;
    };
}