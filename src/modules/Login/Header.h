#pragma once

#include "../ModuleList.h"
#include "../../storage/models/Authorization.h"
#include "../../widgets/KairosAvatar.h"

#include <gtkmm.h>

namespace EGL3::Modules::Login {
    class HeaderModule : public BaseModule {
    public:
        HeaderModule(ModuleList& Ctx);

        void Hide();

        void Show(const Storage::Models::AuthUserData& Data);

    private:
        Gtk::StackSwitcher& Switcher;
        Gtk::MenuButton& MenuButton;
        Gtk::Label& Username;
        Gtk::AspectFrame& AvatarContainer;
        Widgets::KairosAvatar Avatar;
    };
}