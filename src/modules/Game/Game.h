#pragma once

#include "../../storage/models/InstalledGame.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../Login/Auth.h"
#include "Download.h"
#include "Play.h"
#include "UpdateCheck.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class GameModule : public BaseModule {
    public:
        GameModule(ModuleList& Ctx);

        enum class State {
            Unknown,
            Play,
            Update,
            Install,

            Playing,
            Updating,
            Installing,
            Uninstalling
        };

    private:
        void UpdateToState(const char* NewLabel, bool Playable = false, bool Menuable = false);

        void OnUpdateToCurrentState();

        void UpdateToCurrentState();

        void PrimaryButtonClicked();

        void CleanInstalls();

        Storage::Models::InstalledGame* GetInstall(Storage::Game::GameId Id);

        struct StateHolder {
            StateHolder(GameModule& Module, State HeldState = State::Unknown);
            ~StateHolder();

            StateHolder(const StateHolder&) = delete;
            StateHolder& operator=(const StateHolder&) = delete;

            bool HasHeldState() const noexcept {
                return GetHeldState() != State::Unknown;
            }

            State GetHeldState() const noexcept {
                return HeldState;
            }

            void SetHeldState(State NewHeldState);

            void ClearHeldState();

            Utils::Callback<void()> Clicked;
        private:
            State HeldState;
            GameModule* Module;
        };

        struct StateHolderComparer {
            bool operator()(const StateHolder* Left, const StateHolder* Right) const {
                return Left->GetHeldState() < Right->GetHeldState();
            }
        };

        Storage::Persistent::Store& Storage;
        AsyncFFModule& AsyncFF;
        Login::AuthModule& Auth;
        DownloadModule& Download;
        PlayModule& Play;
        UpdateCheckModule& UpdateCheck;

        Gtk::Button& PlayBtn;
        Gtk::MenuButton& PlayMenuBtn;
        Gtk::MenuItem& PlayMenuVerifyOpt;
        Gtk::MenuItem& PlayMenuModifyOpt;
        Gtk::MenuItem& PlayMenuSignOutOpt;

        Glib::Dispatcher CurrentStateDispatcher;
        StateHolder* CurrentStateHolder;
        std::shared_mutex StateHolderMtx;
        std::vector<StateHolder*> StateHolders;

        StateHolder InstallStateHolder;
        StateHolder PlayStateHolder;
    };
}