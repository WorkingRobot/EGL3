#pragma once

#include "../utils/SysTray.h"
#include "../utils/Toasts.h"
#include "../storage/game/GameId.h"
#include "ModuleList.h"

namespace EGL3::Modules {
    class SysTrayModule : public BaseModule {
    public:
        SysTrayModule(ModuleList& Ctx);

        void SetActionLabel(const std::string& Label, bool Clickable);

        void SetLoggedIn(bool IsLoggedIn);

        void ShowToast(const Utils::ToastTemplate& Toast, const Utils::ToastHandler& Handler = {});

        void Present();

        sigc::signal<void()> OnActionClicked;
        Utils::Callback<void()> OnLogIn;
        Utils::Callback<void()> OnLogOut;
        Utils::Callback<bool()> OnQuit;

    private:
        void Construct();

        void MenuStackClicked(Gtk::Widget& StackChild);

        bool IsLoggedIn;

        Glib::RefPtr<Gtk::Application> Application;
        Gtk::ApplicationWindow& Window;
        Gtk::Stack& MainStack;

        bool ShiftPressed;

        Utils::Toasts Toasts;
        std::optional<Utils::SysTray> Tray;

        bool WindowFocusedOutAlready;
        std::optional<Gtk::Window> HiddenWindow;

        Gtk::Menu Container;

        Gtk::MenuItem ItemAction;

        Gtk::SeparatorMenuItem ItemSeparatorA;

        std::vector<Gtk::MenuItem> StackSwitcherItems;

        Gtk::SeparatorMenuItem ItemSeparatorB;

        Gtk::MenuItem ItemLogIn;
        Gtk::MenuItem ItemLogOut;
        Gtk::MenuItem ItemQuit;
    };
}