#include "Options.h"

namespace EGL3::Modules::Game {
    OptionsModule::OptionsModule(ModuleList& Modules, const Utils::GladeBuilder& Builder) :
        Window(Builder.GetWidget<Gtk::Window>("InstallMenu"))
    {
    }

    Gtk::Window& OptionsModule::GetWindow() const
    {
        return Window;
    }
}