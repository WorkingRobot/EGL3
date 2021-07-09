#pragma once

#include "../utils/SysTray.h"
#include "../utils/Toasts.h"
#include "ModuleList.h"

namespace EGL3::Modules {
    class SysTrayModule : public BaseModule {
    public:
        SysTrayModule(ModuleList& Ctx);

        void SetActionLabel(const std::string& Label, bool Clickable);

        void SetLoggedIn(bool IsLoggedIn);

        void ShowToast(const Utils::ToastTemplate& Toast, const Utils::ToastHandler& Handler = {});

        enum class AppState : uint8_t {
            Focused,
            Unfocused,
            Minimized,
            SysTray
        };

        enum class StackTab : uint8_t {
            Download,
            News,
            Account,
            Settings,
            About,
            Unknown
        };

        void SetAppState(AppState NewState, StackTab Tab = StackTab::Unknown);

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
        Gtk::Widget& TabDownload;
        Gtk::Widget& TabNews;
        Gtk::Widget& TabFriends;
        Gtk::Widget& TabSettings;
        Gtk::Widget& TabAbout;

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