#pragma once

#include "../utils/Callback.h"

#include <gtkmm.h>

#include <deque>
#include <array>

namespace EGL3::Widgets {
    template<int LineCount>
    class Graph {
        static constexpr float DataGap = 15;
        static constexpr int SideBuffer = 3;
        static constexpr int LineWidth = 3;

    public:
        Graph(Gtk::DrawingArea& DrawArea) :
            DrawArea(DrawArea),
            OnFormatTooltip([](int Idx, const auto& Data) { return ""; })
        {
            DrawDispatcher.connect([this]() { OnSnapshotAdded(); });

            DrawArea.signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context>& Ctx) { return OnDraw(Ctx); });

            DrawArea.signal_query_tooltip().connect([this](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
                std::lock_guard Guard(DrawMutex);

                if (DrawData.empty()) {
                    return true;
                }

                Gtk::Allocation Allocation = this->DrawArea.get_allocation();
                int Width = Allocation.get_width();

                int Idx = std::clamp<int>(round((Width - x) / DataGap) + SideBuffer, 0, DrawData.size() - 1);
                tooltip->set_text(OnFormatTooltip(Idx, DrawData[Idx]));

                return true;
            });
        }

        ~Graph() {
            std::lock_guard Guard(DrawMutex);
        }

        void Add(const std::array<float, LineCount>& Data) {
            {
                std::lock_guard Guard(DrawMutex);
                DrawData.emplace_front(Data);
            }
            DrawDispatcher.emit();
        }

        void Clear() {
            {
                std::lock_guard Guard(DrawMutex);
                DrawData.clear();
            }
            DrawDispatcher.emit();
        }

        Utils::Callback<Glib::ustring(int, const std::array<float, LineCount>&)> OnFormatTooltip;

    private:
        using Point = std::pair<float, float>;

        static constexpr Point Interpolate(const Point& A, const Point& B, float Ratio = .5f) {
            return { std::lerp(A.first, B.first, Ratio), std::lerp(A.second, B.second, Ratio) };
        }

        template<typename DrawCallback>
        static void ExecuteBSpline(std::vector<Point>& Input, const DrawCallback& OnDraw) {
            std::vector<Point> one_thirds;
            std::vector<Point> two_thirds;
            one_thirds.reserve(Input.size() - 1);
            two_thirds.reserve(Input.size() - 1);

            auto PrevItr = Input.begin();
            for (auto Itr = Input.begin() + 1; Itr != Input.end(); Itr++) {
                one_thirds.emplace_back(Interpolate(*PrevItr, *Itr, 1 / 3.f));
                two_thirds.emplace_back(Interpolate(*Itr, *PrevItr, 1 / 3.f));

                PrevItr = Itr;
            }

            for (size_t i = 0; i < Input.size() - 3; ++i) {
                OnDraw(
                    Interpolate(two_thirds[i], one_thirds[i + 1]),
                    one_thirds[i + 1],
                    two_thirds[i + 1],
                    Interpolate(two_thirds[i + 1], one_thirds[i + 2])
                );
            }
        }

        void OnSnapshotAdded() {
            {
                std::lock_guard Guard(DrawMutex);
                int Width = DrawArea.get_allocation().get_width();
                while (DrawData.size() > (2 * SideBuffer + 1) && DrawData.size() - (2 * SideBuffer + 1) > Width / DataGap) {
                    DrawData.pop_back();
                }
            }

            DrawArea.queue_draw();
        }

        bool OnDraw(const Cairo::RefPtr<Cairo::Context>& Ctx) {
            std::lock_guard Guard(DrawMutex);
            Gtk::Allocation Allocation = DrawArea.get_allocation();
            int Width = Allocation.get_width();
            int Height = Allocation.get_height() - 2 * LineWidth;

            Ctx->scale(1, 1);
            Ctx->set_line_width(LineWidth);

            if (DrawData.size() >= 4) {
                int SnapshotIdx = DrawData.size() - 1;
                std::array<std::vector<Point>, LineCount> SplineInput;
                // For all available data points
                for (auto Itr = DrawData.rbegin(); Itr != DrawData.rend(); Itr++, SnapshotIdx--) {
                    int EmplaceCount = (SnapshotIdx == DrawData.size() - 1 || SnapshotIdx == 0) ? 3 : 1;
                    // For every line
                    for (int LineIdx = 0; LineIdx < LineCount; ++LineIdx) {
                        // The first and last points are added 2 more times (because cubic b-spline)
                        for (int EmplaceIdx = 0; EmplaceIdx < EmplaceCount; ++EmplaceIdx) {
                            SplineInput[LineIdx].emplace_back(Width - ((SnapshotIdx - SideBuffer) * DataGap), LineWidth + Height * (1 - Itr->at(LineIdx)));
                        }
                    }
                }

                for (int LineIdx = 0; LineIdx < LineCount; ++LineIdx) {
                    Gdk::RGBA Color = DrawArea.get_style_context()->get_color();
                    DrawArea.get_style_context()->lookup_color("graph_color_" + std::to_string(LineIdx), Color);
                    Ctx->set_source_rgba(Color.get_red(), Color.get_green(), Color.get_blue(), Color.get_alpha());

                    ExecuteBSpline(SplineInput[LineIdx], [this, LineIdx, Ctx](const Point& start, const Point& a, const Point& b, const Point& c) {
                        Ctx->move_to(start.first, start.second);
                        Ctx->curve_to(a.first, a.second, b.first, b.second, c.first, c.second);
                        Ctx->stroke();
                    });
                }
            }

            return true;
        }

        Gtk::DrawingArea& DrawArea;

        std::mutex DrawMutex;
        std::deque<std::array<float, LineCount>> DrawData;
        Glib::Dispatcher DrawDispatcher;
    };
}
