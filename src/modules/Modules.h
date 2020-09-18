#pragma once

#include "SidebarNotebook.h"
#include "../widgets/WhatsNewItem.h"

namespace EGL3::Modules {
	typedef std::vector<std::unique_ptr<BaseModule>> ModuleList;

	void DeleteModuleList(void* Modules) {
		std::default_delete<ModuleList>()((ModuleList*)Modules);
	}

	void InitializeModules(Gtk::ApplicationWindow& AppWnd, GladeBuilder& Builder) {
		auto Modules = new ModuleList;

		Modules->emplace_back(std::unique_ptr<BaseModule>(new SidebarNotebookModule(Builder)));

		AppWnd.set_data("EGL3Modules", Modules, &DeleteModuleList);
	}
}