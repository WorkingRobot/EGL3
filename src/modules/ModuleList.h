#pragma once

#include "BaseModule.h"
#include "../utils/GladeBuilder.h"
#include "../storage/persistent/Store.h"

#include <gtkmm.h>
#include <any>

namespace EGL3::Modules {
	// TODO: add destructors on modules with a mutex or async thread that locks the mutex to ensure all threads have exited
	class ModuleList {
	public:
		// Run this on the startup signal. It gets deleted automatically when the app shuts down
		static void Attach(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder) {
			new ModuleList(App, Builder);
		}

		template<typename T>
		const T& GetModule() const {
			for (auto& Module : Modules) {
				if (auto Ret = std::any_cast<std::shared_ptr<T>>(&Module)) {
					return **Ret;
				}
			}
		}

		template<typename T>
		T& GetModule() {
			for (auto& Module : Modules) {
				if (auto Ret = std::any_cast<std::shared_ptr<T>>(&Module)) {
					return **Ret;
				}
			}
		}

	private:
		ModuleList(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder) {
			App->set_data("EGL3Modules", this, [](void* Data) {
				delete (ModuleList*)Data;
			});
			App->signal_shutdown().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
				App->remove_data("EGL3Modules");
			}, App));

			AddModules(App, Builder, *(Storage::Persistent::Store*)App->get_data("EGL3Storage"));
		}

		void AddModules(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder, Storage::Persistent::Store& Storage);

		template<typename T, typename... Args>
		void AddModule(Args&&... ModuleArgs) {
			Modules.emplace_back(std::make_shared<T>(std::forward<Args>(ModuleArgs)...));
		}

		std::vector<std::any> Modules;
	};
}
