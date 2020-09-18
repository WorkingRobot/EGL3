#pragma once

#include <gtkmm.h>

#include "assert.h"

namespace EGL3::Utils {
	class GladeBuilder {
	public:
		GladeBuilder(const char* FilePath) {
			Builder = Gtk::Builder::create_from_file(FilePath);
		}

		template<class T>
		T& GetWidget(const char* Name) {
			T* Ret = NULL;
			Builder->get_widget(Name, Ret);
			EGL3_ASSERT(Ret != NULL, "Widget does not exist");
			return std::ref(*Ret);
		}

	private:
		Glib::RefPtr<Gtk::Builder> Builder;
	};
}