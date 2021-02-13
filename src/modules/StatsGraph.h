#pragma once

#include "BaseModule.h"

#include "../utils/GladeBuilder.h"
#include "../widgets/Graph.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    class StatsGraphModule : public BaseModule {
    public:
        StatsGraphModule(const Utils::GladeBuilder& Builder);

        ~StatsGraphModule();

    private:
        sigc::connection TimerConnection;
        Widgets::Graph<1> Graph;
        float n = 0;
    };
}