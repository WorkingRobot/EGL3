#pragma once

#include "../../utils/Callback.h"
#include "../../utils/GladeBuilder.h"
#include "../BaseModule.h"
#include "../ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules::Game {
    struct InstallData {
        bool Cancelled;
        std::vector<std::string> Ids; // UniqueId of all elements installed
    };

    class OptionsModule : public BaseModule {
    public:
        OptionsModule(ModuleList& Modules, const Utils::GladeBuilder& Builder);

        Gtk::Window& GetWindow() const;

        Utils::Callback<void(const InstallData&)> OnActionClicked;

    private:
        Gtk::Window& Window;
    };
}