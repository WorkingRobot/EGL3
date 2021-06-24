#include "Stack.h"

namespace EGL3::Modules::Login {
    StackModule::StackModule(ModuleList& Ctx) :
        Stack(Ctx.GetWidget<Gtk::Stack>("LoginStateStack")),
        SignInContainer(Ctx.GetWidget<Gtk::Box>("LoginStateChooser")),
        IntermediateContainer(Ctx.GetWidget<Gtk::Box>("LoginStateIntermediate")),
        PrimaryContainer(Ctx.GetWidget<Gtk::Box>("MainContainer")),
        IntermediateUsername(Ctx.GetWidget<Gtk::Label>("LoggingInUsername")),
        IntermediateAvatarContainer(Ctx.GetWidget<Gtk::AspectFrame>("LoggingInAvatarContainer")),
        IntermediateAvatar(Ctx.GetModule<Modules::ImageCacheModule>(), 64)
    {
        IntermediateAvatarContainer.add(IntermediateAvatar);
        IntermediateAvatar.show();

        DisplaySignIn();
    }

    void StackModule::DisplaySignIn()
    {
        Stack.set_visible_child(SignInContainer);
    }

    void StackModule::DisplayIntermediate(const Storage::Models::AuthUserData& UserData)
    {
        IntermediateUsername.set_text(UserData.DisplayName);
        IntermediateAvatar.set_avatar(UserData.KairosAvatar, UserData.KairosBackground);

        Stack.set_visible_child(IntermediateContainer);
    }

    void StackModule::DisplayPrimary()
    {
        Stack.set_visible_child(PrimaryContainer);
    }
}