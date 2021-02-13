#include "InstallLocationDialog.h"

namespace EGL3::Widgets {
    InstallLocationDialog::InstallLocationDialog(Gtk::Window& Parent) :
        Dialog(Parent, "Select Install File", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE),
        DialogOpen(false)
    {
        Construct();
    }

    InstallLocationDialog::operator Gtk::Window& () {
        return Dialog;
    }

    void InstallLocationDialog::Show()
    {
        if (!DialogOpen) {
            Dialog.show();
            DialogOpen = true;
        }
        else {
            Dialog.present();
        }
    }

    void InstallLocationDialog::SetLocation(const std::string& NewLocation)
    {
        Dialog.set_filename(NewLocation);
        LocationChosen(NewLocation);
    }

    void InstallLocationDialog::Construct()
    {
        Dialog.signal_response().connect([this](int Resp) {
            if (Resp == Gtk::RESPONSE_OK) {
                LocationChosen(Dialog.get_filename());
            }
            Dialog.hide();
            DialogOpen = false;
        });

        Dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
        Dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

        Dialog.set_do_overwrite_confirmation(true);
        Dialog.set_current_name("Fortnite.egia");
        Dialog.set_modal(true);

        auto FilterEgia = Gtk::FileFilter::create();
        FilterEgia->set_name("Epic Games Install Archives (*.egia)");
        FilterEgia->add_pattern("*.egia");
        Dialog.add_filter(FilterEgia);

        auto FilterAny = Gtk::FileFilter::create();
        FilterAny->set_name("All Files (*.*)");
        FilterAny->add_pattern("*");
        Dialog.add_filter(FilterAny);
    }
}
