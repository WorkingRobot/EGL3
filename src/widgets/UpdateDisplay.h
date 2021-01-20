#pragma once

#include "../storage/models/UpdateInfo.h"
#include "../storage/models/UpdateStatsInfo.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class UpdateDisplay {
    public:
        UpdateDisplay();

        operator Gtk::Widget& ();

        void Update(Storage::Models::UpdateStatsInfo& NewStats);

    private:
        void Construct();

        Gtk::Box BaseContainer{ Gtk::ORIENTATION_VERTICAL };
    };
}
