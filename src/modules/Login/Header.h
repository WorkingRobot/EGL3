#pragma once

#include "../ModuleList.h"
#include "../../widgets/AccountList.h"
#include "../../widgets/KairosAvatar.h"

#include <gtkmm.h>

namespace EGL3::Modules::Login {
    class HeaderModule : public BaseModule {
    public:
        HeaderModule(ModuleList& Ctx);

        void Hide();

        void Show(const Storage::Models::AuthUserData& Data);

    private:
        AuthModule& Auth;

        Gtk::StackSwitcher& Switcher;
        Gtk::Box& LoginInfo;
        Gtk::Label& Username;
        Gtk::AspectFrame& AvatarContainer;
        Gtk::EventBox& EventBox;
        Gtk::Revealer& SelectIconRevealer;
        Widgets::KairosAvatar Avatar;

        bool Focused;
        Gtk::Window& AccountListWindow;
        Widgets::AccountList AccountList;

        Glib::Dispatcher LoggedInDispatcher;
    };
}