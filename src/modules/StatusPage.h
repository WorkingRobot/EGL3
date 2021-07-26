#pragma once

#include "../utils/DataDispatcher.h"
#include "../utils/SlotHolder.h"
#include "../web/Response.h"
#include "../web/epic/responses/GetStatuspageSummary.h"
#include "ModuleList.h"

#include <future>
#include <gtkmm.h>

namespace EGL3::Modules {
    class StatusPageModule : public BaseModule {
    public:
        StatusPageModule(ModuleList& Ctx);

        void Refresh();

    private:
        void UpdateLabels(const Web::Response<Web::Epic::Responses::GetStatuspageSummary>& Data);

        void UpdateLabel(Gtk::Label& Label, const std::string& Status);

        Gtk::Button& RefreshBtn;
        Gtk::EventBox& LabelTitleEventBox;
        Gtk::Label& LabelTitle;
        Gtk::Label& LabelFortnite;
        Gtk::Label& LabelEOS;
        Gtk::Label& LabelEGS;

        Utils::SlotHolder SlotRefresh;
        Utils::SlotHolder SlotHoverPointer;
        Utils::SlotHolder SlotOpenInBrowser;

        std::future<void> RefreshTask;
        Utils::DataDispatcher<Web::Response<Web::Epic::Responses::GetStatuspageSummary>> Dispatcher;
    };
}