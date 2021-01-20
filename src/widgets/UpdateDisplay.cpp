#include "UpdateDisplay.h"

namespace EGL3::Widgets {
    UpdateDisplay::UpdateDisplay()
    {
        Construct();
    }

    UpdateDisplay::operator Gtk::Widget& () {
        return BaseContainer;
    }

    void UpdateDisplay::Update(Storage::Models::UpdateStatsInfo& NewStats)
    {

    }

    void UpdateDisplay::Construct()
    {
        BaseContainer.show_all();
    }
}
