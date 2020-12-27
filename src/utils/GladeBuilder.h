#pragma once

#include "Assert.h"

#include <gtkmm.h>

namespace EGL3::Utils {
	class GladeBuilder {
	public:
		GladeBuilder(const char* FilePath) {
			Builder = Gtk::Builder::create_from_file(FilePath);
		}

		template<class T>
		T& GetWidget(const char* Name) const {
			T* Ret = nullptr;
			Builder->get_widget(Name, Ret);
			EGL3_ASSERT(Ret, "Widget does not exist");
			return std::ref(*Ret);
		}

	private:
		Glib::RefPtr<Gtk::Builder> Builder;
	};
}