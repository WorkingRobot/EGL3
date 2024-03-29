#include "StatsGraph.h"

namespace EGL3::Modules {
    StatsGraphModule::StatsGraphModule(ModuleList& Ctx) :
        Graph(Ctx.GetWidget<Gtk::DrawingArea>("StatsGraph"))
    {
        Graph.OnFormatTooltip = [](int Idx, const auto& Data) {
            return std::format("{}, {:.2f}", Idx, Data[0]);
        };

        TimerConnection = Glib::signal_timeout().connect([this]() {
            n += (rand() & 0xff) / 255.f * .4f - 0.2f;
            n = std::clamp(n, 0.f, 1.f);

            Graph.Add({ n });
            return true;
        }, 1000, Glib::PRIORITY_HIGH_IDLE);
    }

    StatsGraphModule::~StatsGraphModule() {
        TimerConnection.disconnect();
    }
}