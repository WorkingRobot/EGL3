#include "StatsGraph.h"

#include <iomanip>

namespace EGL3::Modules {
    StatsGraphModule::StatsGraphModule(const Utils::GladeBuilder& Builder) :
        DrawArea(Builder.GetWidget<Gtk::DrawingArea>("StatsGraph"))
    {
        DrawArea.signal_query_tooltip().connect([this](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
            if (Snapshots.empty()) {
                return true;
            }
            Gtk::Allocation allocation = DrawArea.get_allocation();
            const int width = allocation.get_width();
            const int height = allocation.get_height();

            int i = round((width - x) / 15.f);

            std::lock_guard Guard(SnapshotMutex);
            if (i < 0) {
                i = 0;
            }
            else if (i >= Snapshots.size()) {
                i = Snapshots.size() - 1;
            }
            tooltip->set_text(Glib::ustring::compose("%1, %2", i, Glib::ustring::format(std::setprecision(2), Snapshots[i].Height)));

            return true;
        });

        DrawArea.signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context>& Ctx) {
            std::lock_guard Guard(SnapshotMutex);
            Gtk::Allocation allocation = DrawArea.get_allocation();
            const int width = allocation.get_width();
            const int height = allocation.get_height();

            Ctx->scale(1, 1);
            Ctx->set_line_width(3);
            Gdk::RGBA Color = DrawArea.get_style_context()->get_color();
            Ctx->set_source_rgba(Color.get_red(), Color.get_green(), Color.get_blue(), Color.get_alpha());

            if (Snapshots.size() >= 4) {
                int i = Snapshots.size() - 1;
                std::vector<point_t> SplineInput;
                for (auto Itr = Snapshots.rbegin(); Itr != Snapshots.rend(); Itr++) {
                    if (i == Snapshots.size() - 1 || i == 0) {
                        SplineInput.emplace_back(width - (i * 15), height - Itr->Height * height);
                        SplineInput.emplace_back(width - (i * 15), height - Itr->Height * height);
                    }
                    SplineInput.emplace_back(width - (i * 15), height - Itr->Height * height);
                    i--;
                }
                ExecuteBSpline(SplineInput, [Ctx](auto& start, auto& a, auto& b, auto& c) {
                    Ctx->move_to(start.first, start.second);
                    Ctx->curve_to(a.first, a.second, b.first, b.second, c.first, c.second);
                    Ctx->stroke();
                });
            }

            return true;
        });

        TimerConnection = Glib::signal_timeout().connect([this]() {
            std::lock_guard Guard(SnapshotMutex);

            n += (rand() & 0xff)/255.f*.4f-0.2f;
            n = std::min(std::max(n, 0.f), 1.f);
            const int width = DrawArea.get_allocation().get_width();
            while (Snapshots.size() > width / 15) {
                Snapshots.pop_back();
            }
            Snapshots.emplace_front(Snapshot{ n });

            DrawArea.queue_draw();

            return true;
        }, 1000, Glib::PRIORITY_HIGH_IDLE);
    }

    StatsGraphModule::~StatsGraphModule() {
        TimerConnection.disconnect();
        std::lock_guard Guard(SnapshotMutex);
    }

    template<typename DrawCallback>
    void StatsGraphModule::ExecuteBSpline(std::vector<point_t>& bspline, const DrawCallback& draw_cb) {
        std::vector<point_t> one_thirds;
        std::vector<point_t> two_thirds;
        one_thirds.reserve(bspline.size() - 1);
        two_thirds.reserve(bspline.size() - 1);

        auto PrevItr = bspline.begin();
        for (auto Itr = bspline.begin() + 1; Itr != bspline.end(); Itr++) {
            one_thirds.emplace_back(Interpolate(*PrevItr, *Itr, 1 / 3.f));
            two_thirds.emplace_back(Interpolate(*Itr, *PrevItr, 1 / 3.f));

            PrevItr = Itr;
        }

        for (int i = 0; i < bspline.size() - 3; ++i) {
            draw_cb(
                Interpolate(two_thirds[i], one_thirds[i + 1]),
                one_thirds[i + 1],
                two_thirds[i + 1],
                Interpolate(two_thirds[i + 1], one_thirds[i + 2])
            );
        }
    }

    StatsGraphModule::point_t StatsGraphModule::Interpolate(const point_t& A, const point_t& B, float ratio) {
        return { A.first * (1.0 - ratio) + B.first * ratio, A.second * (1.0 - ratio) + B.second * ratio };
    }
}