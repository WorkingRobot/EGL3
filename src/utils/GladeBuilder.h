#pragma once

#include "Assert.h"

#include <filesystem>
#include <gtkmm.h>

namespace EGL3::Utils {
    class GladeBuilder {
    public:
        GladeBuilder(const std::filesystem::path& Path) :
            Builder(Gtk::Builder::create_from_file(Path.string()))
        {

        }

        template<class T>
        T& GetWidget(const char* Name) const {
            T* Ret = nullptr;
            Builder->get_widget(Name, Ret);
            EGL3_CONDITIONAL_LOG(Ret, LogLevel::Critical, "Widget does not exist. The UI file is invalid, corrupt, or misplaced.");
            return std::ref(*Ret);
        }

    private:
        Glib::RefPtr<Gtk::Builder> Builder;
    };
}