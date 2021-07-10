#include "SysTray.h"

#include "../utils/Interop.h"
#include "Game/GameInfo.h"

namespace EGL3::Modules {
    SysTrayModule::SysTrayModule(ModuleList& Ctx) :
        OnQuit([]() {return false; }),
        IsLoggedIn(false),
        Window(Ctx.GetWidget<Gtk::ApplicationWindow>("EGL3App")),
        MainStack(Ctx.GetWidget<Gtk::Stack>("MainStack")),
        TabDownload(Ctx.GetWidget<Gtk::Widget>("DownloadStack")),
        TabNews(Ctx.GetWidget<Gtk::Widget>("NewsContainer")),
        TabFriends(Ctx.GetWidget<Gtk::Widget>("AccountsContainer")),
        TabSettings(Ctx.GetWidget<Gtk::Widget>("SettingsContainer")),
        TabAbout(Ctx.GetWidget<Gtk::Widget>("AboutContainer")),
        ShiftPressed(false),
        WindowFocusedOutAlready(false)
    {
        Window.signal_delete_event().connect([this](GdkEventAny* Event) {
            if (ShiftPressed) {
                ItemQuit.activate();
            }
            else {
                Window.hide();
            }
            return true;
        });

        ItemAction.signal_activate().connect([this]() { OnActionClicked.emit(); });
        ItemLogIn.signal_activate().connect([this]() { SetAppState(AppState::Focused); OnLogIn(); });
        ItemLogOut.signal_activate().connect([this]() { SetAppState(AppState::Focused); OnLogOut(); });
        ItemQuit.signal_activate().connect([this]() {
            if (!OnQuit()) {
                Quit();
            }
        });

        ItemSeparatorA.set_label("\xe2\xb8\xbb"); // triple em dash
        ItemSeparatorB.set_label("\xe2\xb8\xbb");
        ItemLogIn.set_label("Log In");
        ItemLogOut.set_label("Log Out");
        ItemQuit.set_label("Quit");

        Window.signal_key_press_event().connect([this](GdkEventKey* Event) {
            ShiftPressed = Event->state & Gdk::SHIFT_MASK;
            return false;
        });

        Window.signal_key_release_event().connect([this](GdkEventKey* Event) {
            ShiftPressed = Event->state & Gdk::SHIFT_MASK;
            return false;
        });

        Window.signal_realize().connect([this]() {
            Application = Window.get_application();
            Application->hold();

            Tray.emplace(Window);

            Tray->OnClicked.Set([this](int X, int Y) {
                SetAppState(AppState::Focused);
            });

            Tray->OnContextMenu.Set([this](int X, int Y) {
                Construct();

                HiddenWindow.emplace(Gtk::WINDOW_TOPLEVEL);
                HiddenWindow->set_resizable(false);
                HiddenWindow->set_decorated(false);
                HiddenWindow->set_skip_taskbar_hint(true);
                HiddenWindow->set_skip_pager_hint(true);
                HiddenWindow->set_size_request(0, 0);
                HiddenWindow->set_position(Gtk::WIN_POS_MOUSE);

                HiddenWindow->show_all();

                {
                    // Hide window from taskbar (Gtk::WINDOW_TOOLTIP hides it, but it doesn't have the right behavior that Gtk::WINDOW_TOPLEVEL has that I'm looking for)
                    HWND Hwnd = Utils::Interop::GetHwnd(*HiddenWindow);
                    ShowWindow(Hwnd, SW_HIDE);
                    SetWindowLong(Hwnd, GWL_EXSTYLE, GetWindowLong(Hwnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
                    ShowWindow(Hwnd, SW_SHOW);
                }

                HiddenWindow->grab_focus();

                WindowFocusedOutAlready = false;
                HiddenWindow->set_events(Gdk::FOCUS_CHANGE_MASK);
                HiddenWindow->signal_focus_out_event().connect([this](GdkEventFocus* Event) {
                    if (WindowFocusedOutAlready) {
                        HiddenWindow->hide();
                        HiddenWindow.reset();
                    }
                    else {
                        WindowFocusedOutAlready = true;
                    }
                    return true;
                });

                Container.popup(2, GDK_CURRENT_TIME);
            });
        });
    }

    void SysTrayModule::SetActionLabel(const std::string& Label, bool Clickable)
    {
        ItemAction.set_label(Label);
        ItemAction.set_sensitive(Clickable);
    }

    void SysTrayModule::SetLoggedIn(bool IsLoggedIn)
    {
        this->IsLoggedIn = IsLoggedIn;
    }

    void SysTrayModule::ShowToast(const Utils::ToastTemplate& Toast, const Utils::ToastHandler& Handler)
    {
        Toasts.ShowToast(Toast, Handler);
    }

    void SysTrayModule::SetAppState(AppState NewState, StackTab Tab)
    {
        switch (NewState)
        {
        case AppState::Focused:
            Window.present();
            break;
        case AppState::Unfocused:
            Window.show();
            Window.deiconify();
            Window.unset_focus();
            break;
        case AppState::Minimized:
            Window.show();
            Window.unset_focus();
            Window.iconify();
            break;
        case AppState::SysTray:
            Window.hide();
            break;
        }

        switch (Tab)
        {
        case StackTab::Download:
            MainStack.set_visible_child(TabDownload);
            break;
        case StackTab::News:
            MainStack.set_visible_child(TabNews);
            break;
        case StackTab::Account:
            MainStack.set_visible_child(TabFriends);
            break;
        case StackTab::Settings:
            MainStack.set_visible_child(TabSettings);
            break;
        case StackTab::About:
            MainStack.set_visible_child(TabAbout);
            break;
        }
    }

    void SysTrayModule::Quit()
    {
        Window.hide();
        Application->release();
    }

    void SysTrayModule::Construct()
    {
        Container.foreach([this](Gtk::Widget& Child) {
            Container.remove(Child);
        });
        StackSwitcherItems.clear();

        if (IsLoggedIn && !OnActionClicked.empty()) {
            Container.add(ItemAction);
            Container.add(ItemSeparatorA);
        }

        if (IsLoggedIn) {
            MainStack.foreach([this](Gtk::Widget& Child) {
                if (!Child.get_visible()) {
                    return;
                }

                auto& Item = StackSwitcherItems.emplace_back();
                Item.set_label(MainStack.child_property_title(Child));
                Item.signal_activate().connect([this, &Child]() { SetAppState(AppState::Focused); MenuStackClicked(Child); });
                Container.add(Item);
            });

            Container.add(ItemSeparatorB);
        }

        Container.add(!IsLoggedIn ? ItemLogIn : ItemLogOut);
        Container.add(ItemQuit);

        Container.show_all();
    }

    void SysTrayModule::MenuStackClicked(Gtk::Widget& StackChild)
    {
        MainStack.set_visible_child(StackChild);
    }
}