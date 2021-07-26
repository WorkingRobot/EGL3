#include "StatusPage.h"

#include "../utils/OpenBrowser.h"
#include "../web/epic/EpicClient.h"
#include "../web/Hosts.h"

namespace EGL3::Modules {
    StatusPageModule::StatusPageModule(ModuleList& Ctx) :
        RefreshBtn(Ctx.GetWidget<Gtk::Button>("StatusPageRefreshBtn")),
        LabelTitleEventBox(Ctx.GetWidget<Gtk::EventBox>("StatusPageTitleEventBox")),
        LabelTitle(Ctx.GetWidget<Gtk::Label>("StatusPageTitle")),
        LabelFortnite(Ctx.GetWidget<Gtk::Label>("StatusPageFortnite")),
        LabelEOS(Ctx.GetWidget<Gtk::Label>("StatusPageEOS")),
        LabelEGS(Ctx.GetWidget<Gtk::Label>("StatusPageEGS"))
    {
        Dispatcher.connect([this](const Web::Response<Web::Epic::Responses::GetStatuspageSummary>& Data) { UpdateLabels(Data); });
        SlotRefresh = RefreshBtn.signal_clicked().connect([this]() { Refresh(); });
        SlotHoverPointer = LabelTitleEventBox.signal_enter_notify_event().connect([this](GdkEventCrossing* Evt) {
            auto Wnd = LabelTitle.get_window();
            Wnd->set_cursor(Gdk::Cursor::create(Wnd->get_display(), "pointer"));
            return true;
        });
        SlotOpenInBrowser = LabelTitleEventBox.signal_button_release_event().connect([this](GdkEventButton* Evt) {
            Utils::OpenInBrowser(Web::GetHostUrl<Web::Host::StatuspageNonApi>());
            return true;
        });

        Refresh();
    }

    void StatusPageModule::Refresh() {
        if (!RefreshBtn.get_sensitive()) {
            return;
        }

        RefreshBtn.set_sensitive(false);

        UpdateLabel(LabelFortnite, "Refreshing");
        UpdateLabel(LabelEOS, "Refreshing");
        UpdateLabel(LabelEGS, "Refreshing");

        RefreshTask = std::async(std::launch::async, [this]() {
            Dispatcher.emit(Web::Epic::EpicClient().GetStatuspageSummary());
        });
    }

    void StatusPageModule::UpdateLabels(const Web::Response<Web::Epic::Responses::GetStatuspageSummary>& Data) {
        if (!Data.HasError()) {
            for (auto& Component : Data->Components) {
                if (Component.Id == "wf1ys2kx4pxc") {
                    UpdateLabel(LabelFortnite, Component.Status);
                }
                else if (Component.Id == "zwclljjbtmfs") {
                    UpdateLabel(LabelEOS, Component.Status);
                }
                else if (Component.Id == "4n43gb11j5v5") {
                    UpdateLabel(LabelEGS, Component.Status);
                }
            }
        }

        RefreshBtn.set_sensitive(true);
    }

    void StatusPageModule::UpdateLabel(Gtk::Label& Label, const std::string& Status) {
        if (Status == "operational") {
            Label.set_text("Operational");
            Label.get_style_context()->add_class("status-operational");
        }
        else if (Status == "under_maintenance") {
            Label.set_text("Under Maintenance");
            Label.get_style_context()->add_class("status-maintenance");
        }
        else if (Status == "degraded_performance") {
            Label.set_text("Degraded Performance");
            Label.get_style_context()->add_class("status-degraded");
        }
        else if (Status == "partial_outage") {
            Label.set_text("Partial Outage");
            Label.get_style_context()->add_class("status-partial");
        }
        else if (Status == "major_outage") {
            Label.set_text("Major Outage");
            Label.get_style_context()->add_class("status-major");
        }
        else {
            Label.set_text(Status);
            // using list_classes() causes a heap corruption when the
            // list is destructed (at least when this runs for the first time)
            Label.get_style_context()->remove_class("status-operational");
            Label.get_style_context()->remove_class("status-maintenance");
            Label.get_style_context()->remove_class("status-degraded");
            Label.get_style_context()->remove_class("status-partial");
            Label.get_style_context()->remove_class("status-major");
        }
    }
}