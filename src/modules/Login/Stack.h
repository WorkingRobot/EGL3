#pragma once

#include "../ModuleList.h"
#include "../../widgets/KairosAvatar.h"

#include <gtkmm.h>

namespace EGL3::Modules::Login {
    class StackModule : public BaseModule {
    public:
        StackModule(ModuleList& Ctx);

        void DisplaySignIn();

        void DisplayIntermediate(const Storage::Models::AuthUserData& UserData);

        void DisplayPrimary();

    private:
        Gtk::Stack& Stack;
        Gtk::Box& SignInContainer;
        Gtk::Box& IntermediateContainer;
        Gtk::Box& PrimaryContainer;

        Gtk::Label& IntermediateUsername;
        Gtk::AspectFrame& IntermediateAvatarContainer;
        Widgets::KairosAvatar IntermediateAvatar;
    };
}