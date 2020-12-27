#pragma once

#include "../utils/GladeBuilder.h"
#include "BaseModule.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    class StatsGraphModule : public BaseModule {
    public:
        StatsGraphModule(const Utils::GladeBuilder& Builder);

        ~StatsGraphModule();

    private:
        Gtk::DrawingArea& DrawArea;

        // https://stackoverflow.com/a/2539718
        using point_t = std::pair<float, float>;
        template<typename DrawCallback>
        static void ExecuteBSpline(std::vector<point_t>& bspline, const DrawCallback& draw_cb);

        static point_t Interpolate(const point_t& A, const point_t& B, float ratio = .5f);

        struct Snapshot {
            float Height;
        };

        sigc::connection TimerConnection;
        std::mutex SnapshotMutex;
        std::deque<Snapshot> Snapshots;
        float n = 0;
    };
}