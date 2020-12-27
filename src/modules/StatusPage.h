#pragma once

#include "../utils/GladeBuilder.h"
#include "../web/BaseClient.h"
#include "../web/epic/responses/GetStatuspageSummary.h"
#include "BaseModule.h"

#include <future>
#include <gtkmm.h>

namespace EGL3::Modules {
	class StatusPageModule : public BaseModule {
	public:
		StatusPageModule(const Utils::GladeBuilder& Builder);

		void Refresh();

	private:
		void UpdateLabels();

		void UpdateLabel(Gtk::Label& Label, std::string& Status);

		Gtk::Button& RefreshBtn;
		Gtk::EventBox& LabelTitleEventBox;
		Gtk::Label& LabelTitle;
		Gtk::Label& LabelFortnite;
		Gtk::Label& LabelEOS;
		Gtk::Label& LabelEGS;

		std::future<void> RefreshTask;
		Glib::Dispatcher Dispatcher;
		Web::BaseClient::Response<Web::Epic::Responses::GetStatuspageSummary> Data;
	};
}