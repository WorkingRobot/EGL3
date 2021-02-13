#pragma once

#include "../utils/Callback.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class InstallLocationDialog {
    public:
        InstallLocationDialog(Gtk::Window& Parent);

        operator Gtk::Window& ();

        void Show();

        void SetLocation(const std::string& NewLocation);

        // Runs in UI thread
        Utils::Callback<void(const std::string&)> LocationChosen;

    private:
        void Construct();

        bool DialogOpen;
        Gtk::FileChooserDialog Dialog;
    };
}
