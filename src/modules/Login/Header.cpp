#include "Header.h"

namespace EGL3::Modules::Login {
    HeaderModule::HeaderModule(ModuleList& Ctx) :
        Switcher(Ctx.GetWidget<Gtk::StackSwitcher>("MainStackSwitcher")),
        LoginInfo(Ctx.GetWidget<Gtk::Box>("LoginInfoContainer")),
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
        LoginInfo.hide();
    }

    void HeaderModule::Show(const Storage::Models::AuthUserData& Data)
    {
        Switcher.show();
        Username.set_text(Data.DisplayName);
        Avatar.set_avatar(Data.KairosAvatar, Data.KairosBackground);
        LoginInfo.show();
    }
}