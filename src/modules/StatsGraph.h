#pragma once

#include "../widgets/Graph.h"
#include "ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    class StatsGraphModule : public BaseModule {
    public:
        StatsGraphModule(ModuleList& Ctx);

        ~StatsGraphModule();

    private:
        sigc::connection TimerConnection;
        Widgets::Graph<1> Graph;
        float n = 0;
    };
}