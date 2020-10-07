#pragma once

#include "SidebarNotebook.h"
#include "WhatsNew.h"

namespace EGL3::Modules {

	void Initialize(const Glib::RefPtr<Gtk::Application>& App, Utils::GladeBuilder& Builder) {
		auto Modules = new std::vector<std::unique_ptr<BaseModule>>;

		Modules->emplace_back(std::unique_ptr<BaseModule>(new SidebarNotebookModule(Builder)));
		Modules->emplace_back(std::unique_ptr<BaseModule>(new WhatsNewModule(App, Builder)));

		App->set_data("EGL3Modules", Modules, [](void* Data) {
			delete (decltype(Modules))Data;
		});
	}

	void Destroy(const Glib::RefPtr<Gtk::Application>& App) {
		App->remove_data("EGL3Modules");
	}
}
