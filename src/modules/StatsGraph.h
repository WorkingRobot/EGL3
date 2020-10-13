#pragma once

#include "BaseModule.h"

#include <gtkmm.h>
#include <gdk/gdk.h>
#include <variant>

#include "../storage/models/WhatsNew.h"
#include "../storage/persistent/Store.h"
#include "../utils/GladeBuilder.h"
#include "../utils/OpenBrowser.h"
#include "../web/epic/EpicClient.h"
#include "../widgets/WhatsNewItem.h"

namespace EGL3::Modules {
	class StatsGraphModule : public BaseModule {
	public:
		StatsGraphModule(Utils::GladeBuilder& Builder) :
			DrawArea(Builder.GetWidget<Gtk::DrawingArea>("StatsGraph"))
		{
			DrawArea.signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context>& Ctx) {
				std::lock_guard Guard(SnapshotMutex);
				Gtk::Allocation allocation = DrawArea.get_allocation();
				const int width = allocation.get_width();
				const int height = allocation.get_height();

				// scale to unit square (0 to 1 width and height)
				Ctx->scale(1, 1);
				Ctx->set_line_width(3);
				Ctx->set_source_rgb(1, 1, 1);
				// draw curve
				int i = 0;
				double x1 = 0, y1 = 0;
				double x2 = 0, y2 = 0;
				decltype(Snapshots.rend()) PrevItr = Snapshots.rend();
				for (auto Itr = Snapshots.rbegin(); Itr != Snapshots.rend(); Itr++, i+=10) {
					x1 = width - i;
					y1 = height - (1 * Itr->Height) * (height * 5);
					if (PrevItr != Snapshots.rend()) {
						Ctx->curve_to(std::lerp(x1, x2, .5), std::lerp(y1, y2, .5), std::lerp(x1, x2, .5), std::lerp(y1, y2, .5), x1, y1);
					}
					else {
						Ctx->move_to(x1, y1);
					}
					PrevItr = Itr;
					x2 = x1;
					y2 = y1;
				}
				Ctx->stroke();

				return true;
			});

			Glib::signal_timeout().connect([this]() {
				std::lock_guard Guard(SnapshotMutex);

				n += (rand() & 0xff)/255.f*.1f-0.05f;
				n = std::max(n, 0.f);
				Snapshots.emplace_back(Snapshot{ n });

				DrawArea.queue_draw();

				return true;
			}, 100, Glib::PRIORITY_HIGH_IDLE);
		}

	private:
		Gtk::DrawingArea& DrawArea;

		struct Snapshot {
			float Height;
		};

		std::mutex SnapshotMutex;
		std::deque<Snapshot> Snapshots;
		float n = 0;
	};
}