#pragma once

#include "../ModuleList.h"
#include "../../widgets/AccountList.h"
#include "../../utils/egl/RememberMe.h"
#include "Auth.h"

#include <gtkmm.h>

namespace EGL3::Modules::Login {
    class ChooserModule : public BaseModule {
    public:
        ChooserModule(ModuleList& Ctx);

    private:
        AuthModule& Auth;

        Gtk::Image& Icon;

        Widgets::AccountList AccountList;
    };
}