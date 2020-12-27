#pragma once

#include "../utils/GladeBuilder.h"
#include "BaseModule.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    class GameModule : public BaseModule {
    public:
        GameModule(const Utils::GladeBuilder& Builder);

    private:
        
    };
}