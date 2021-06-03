#include "Header.h"

namespace EGL3::Modules::Login {
    HeaderModule::HeaderModule(ModuleList& Ctx) :
        Switcher(Ctx.GetWidget<Gtk::StackSwitcher>("MainStackSwitcher")),
        MenuButton(Ctx.GetWidget<Gtk::MenuButton>("LoginInfoHeader")),
        Username(Ctx.GetWidget<Gtk::Label>("LoginInfoUsername")),
        AvatarContainer(Ctx.GetWidget<Gtk::AspectFrame>("LoginInfoAvatarContainer")),
        Avatar(Ctx.GetModule<Modules::ImageCacheModule>(), 32)
    {
        AvatarContainer.add(Avatar);
        Avatar.show();
    }

    void HeaderModule::Hide()
    {
        Switcher.hide();
        MenuButton.hide();
    }

    void HeaderModule::Show(const Storage::Models::AuthUserData& Data)
    {
        Switcher.show();
        Username.set_text(Data.DisplayName);
        Avatar.set_avatar(Data.KairosAvatar, Data.KairosBackground);
        MenuButton.show();
    }
}