#include "InstallLocationDialog.h"

namespace EGL3::Widgets {
    InstallLocationDialog::InstallLocationDialog(Gtk::Window& Parent) :
        Parent(Parent)
    {

    }

    InstallLocationDialog::operator Gtk::Window& () {
        return *Dialog;
    }

    void InstallLocationDialog::Show()
    {
        Construct();
        Dialog->present();
    }

    void InstallLocationDialog::SetLocation(const std::string& NewLocation)
    {
        if (Dialog) {
            Dialog->set_filename(NewLocation);
        }
        Location = NewLocation;
        LocationChosen(NewLocation);
    }

    void InstallLocationDialog::Construct()
    {
        if (Dialog.has_value()) {
            return;
        }

        Dialog.emplace(Parent, "Select Install File", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);

        Dialog->signal_response().connect([this](int Resp) {
            if (Resp == Gtk::RESPONSE_OK) {
                LocationChosen(Dialog->get_filename());
            }
            Dialog->hide();
        });

        Dialog->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
        Dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

        Dialog->set_current_name("Fortnite.egia");
        Dialog->set_modal(true);

        auto FilterEgia = Gtk::FileFilter::create();
        FilterEgia->set_name("Epic Games Install Archives (*.egia)");
        FilterEgia->add_pattern("*.egia");
        Dialog->add_filter(FilterEgia);

        auto FilterAny = Gtk::FileFilter::create();
        FilterAny->set_name("All Files (*.*)");
        FilterAny->add_pattern("*");
        Dialog->add_filter(FilterAny);

        if (!Location.empty()) {
            Dialog->set_filename(Location);
        }
    }
}
