#pragma once

#include "../storage/persistent/Store.h"
#include "../utils/GladeBuilder.h"

#include <any>
#include <gtkmm.h>

namespace EGL3::Modules {
	// TODO: add destructors on modules with a mutex or async thread that locks the mutex to ensure all threads have exited
	class ModuleList {
		ModuleList(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder);

		void AddModules(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder, Storage::Persistent::Store& Storage);

		template<typename T, typename... Args>
		void AddModule(Args&&... ModuleArgs);

	public:
		// Run this on the startup signal. It gets deleted automatically when the app shuts down
		static void Attach(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder);

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
		std::vector<std::any> Modules;
	};
}
