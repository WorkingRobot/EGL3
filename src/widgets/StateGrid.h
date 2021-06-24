#pragma once

#include "../utils/Callback.h"
#include "../utils/SlotHolder.h"

#include <gtkmm.h>

#include <deque>

namespace EGL3::Widgets {
    struct StateColor {
        double R;
        double G;
        double B;
        double A;

        constexpr StateColor(double R, double G, double B, double A = 1.0) : R(R), G(G), B(B), A(A) { }
        constexpr StateColor() : StateColor(0, 0, 0) { }
        // Easily input as 0xAARRGGBB
        constexpr StateColor(uint32_t BGRA) :
            B(((BGRA >>  0) & 0xFF) / 255.),
            G(((BGRA >>  8) & 0xFF) / 255.),
            R(((BGRA >> 16) & 0xFF) / 255.),
            A(((BGRA >> 24) & 0xFF) / 255.)
        {

        }
    };

    template<int RowCount, class CtxT, class StateT, StateT ScheduledState, StateT FinishedState>
    class StateGrid {
    public:
        template<class GetColorT>
        StateGrid(Gtk::DrawingArea& DrawArea, GetColorT&& GetColor) :
            DrawArea(DrawArea),
            GetColor(std::forward<GetColorT>(GetColor))
        {
            DrawDispatcher.connect([this]() { this->DrawArea.queue_draw(); });
            TrimDispatcher.connect([this]() { TryTrimFront(); });

            SlotDraw = DrawArea.signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context>& Ctx) { return OnDraw(Ctx); });
        }

        ~StateGrid() {
            std::lock_guard Guard(DataMtx);
        }

        void Initialize(size_t TotalCount)
        {
            {
                std::lock_guard Guard(DataMtx);

                ExtraItemCount = TotalCount;
                ItemData.clear();
            }
            DrawDispatcher.emit();
        }

        void BeginCtx(const CtxT& Ctx, StateT State)
        {
            {
                std::lock_guard Guard(DataMtx);

                if (EGL3_ENSURE(ExtraItemCount, LogLevel::Warning, "Item count would have been decreased below 0")) {
                    --ExtraItemCount;
                }
                ItemData.emplace_back(Ctx, State);
            }
            DrawDispatcher.emit();
        }

        void UpdateCtx(const CtxT& Ctx, StateT NewState)
        {
            {
                std::lock_guard Guard(DataMtx);

                auto Itr = std::find_if(ItemData.begin(), ItemData.end(), [&Ctx](const auto& Pair) { return Pair.first == Ctx; });
                if (Itr == ItemData.end()) {
                    return;
                }

                Itr->second = NewState;
            }
            DrawDispatcher.emit();
        }

        void EndCtx(const CtxT& Ctx)
        {
            {
                std::lock_guard Guard(DataMtx);

                auto Itr = std::find_if(ItemData.begin(), ItemData.end(), [&Ctx](const auto& Pair) { return Pair.first == Ctx; });
                if (Itr == ItemData.end()) {
                    return;
                }

                Itr->second = FinishedState;
            }
            TrimDispatcher.emit();
            DrawDispatcher.emit();
        }

        Utils::Callback<StateColor(StateT)> GetColor;

    private:
        void TryTrimFront() {
            Gtk::Allocation Allocation = DrawArea.get_allocation();
            int AllocWidth = Allocation.get_width();
            int AllocHeight = Allocation.get_height();

            double SquareSize = (double)AllocHeight / RowCount;
            int ColCount = AllocWidth / SquareSize;
            int ItemsToTrim = ColCount * RowCount;

            std::lock_guard Guard(DataMtx);

            while (ItemData.size() >= ItemsToTrim && std::all_of(ItemData.begin(), ItemData.begin() + ItemsToTrim, [](const auto& Pair) { return Pair.second == FinishedState; })) {
                for (int Idx = 0; Idx < ItemsToTrim; ++Idx) {
                    ItemData.pop_front();
                }
            }
        }

        bool OnDraw(const Cairo::RefPtr<Cairo::Context>& Ctx) {
            Gtk::Allocation Allocation = DrawArea.get_allocation();
            int AllocWidth = Allocation.get_width();
            int AllocHeight = Allocation.get_height();

            double SquareSize = (double)AllocHeight / RowCount;
            int ColCount = AllocWidth / SquareSize;

            Ctx->translate(round((AllocWidth - ColCount * SquareSize) / 2), round((AllocHeight - RowCount * SquareSize) / 2));

            std::lock_guard Guard(DataMtx);

            auto Itr = ItemData.begin();
            auto ExtraItr = ExtraItemCount;
            for (int ColIdx = 0; ColIdx < ColCount; ++ColIdx) {
                for (int RowIdx = 0; RowIdx < RowCount; ++RowIdx) {
                    StateT State;
                    if (Itr != ItemData.end()) {
                        State = Itr->second;
                        ++Itr;
                    }
                    else if (ExtraItr) {
                        State = ScheduledState;
                        --ExtraItr;
                    }
                    else {
                        return true;
                    }
                    auto Color = GetColor(State);
                    Ctx->set_source_rgba(Color.R, Color.G, Color.B, Color.A);
                    Ctx->rectangle(ColIdx * SquareSize, RowIdx * SquareSize, SquareSize, SquareSize);
                    Ctx->fill();
                }
            }

            return true;
        }

        Gtk::DrawingArea& DrawArea;
        Glib::Dispatcher DrawDispatcher;
        Glib::Dispatcher TrimDispatcher;

        Utils::SlotHolder SlotDraw;

        std::mutex DataMtx;
        size_t ExtraItemCount;
        size_t LastRenderedSize;
        std::deque<std::pair<CtxT, StateT>> ItemData;
    };
}
