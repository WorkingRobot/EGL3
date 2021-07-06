#pragma once

#include "../utils/SlotHolder.h"
#include "../utils/SysTray.h"
#include "../storage/game/GameId.h"
#include "ModuleList.h"

namespace EGL3::Modules {
    class SysTrayModule : public BaseModule {
    public:
        SysTrayModule(ModuleList& Ctx);

        void SetInstalledGames(const std::vector<Storage::Game::GameId>& GameIds);

        void SetLoggedIn(bool IsLoggedIn);

        Utils::Callback<void(Storage::Game::GameId GameId)> OnGameClicked;
        Utils::Callback<void()> OnLogIn;
        Utils::Callback<void()> OnLogOut;
        Utils::Callback<bool()> OnQuit;

    private:
        void Construct();

        void Present();

        void MenuStackClicked(Gtk::Widget& StackChild);

        std::vector<Storage::Game::GameId> GameIds;
        bool IsLoggedIn;

        Glib::RefPtr<Gtk::Application> Application;
        Gtk::ApplicationWindow& Window;
        Gtk::Stack& MainStack;

        Utils::SlotHolder SlotShiftPress;
        Utils::SlotHolder SlotShiftRelease;

        bool ShiftPressed;

        std::optional<Utils::SysTray> Tray;
        bool WindowFocusedOutAlready;
        std::optional<Gtk::Window> HiddenWindow;

        Gtk::Menu Container;

        std::vector<Gtk::MenuItem> InstalledGameItems;

        Gtk::SeparatorMenuItem ItemSeparatorA;

        std::vector<Gtk::MenuItem> StackSwitcherItems;

        Gtk::SeparatorMenuItem ItemSeparatorB;

        Gtk::MenuItem ItemLogIn;
        Gtk::MenuItem ItemLogOut;
        Gtk::MenuItem ItemQuit;
    };
}