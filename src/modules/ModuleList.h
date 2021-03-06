#pragma once

#include "../storage/persistent/Store.h"
#include "../utils/Assert.h"
#include "../utils/GladeBuilder.h"

#include <any>
#include <gtkmm.h>

namespace EGL3::Modules {
    class ModuleList {
        ModuleList(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder);

        ~ModuleList();

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
            EGL3_LOG(LogLevel::Critical, "Could not find module");
        }

        template<typename T>
        T& GetModule() {
            for (auto& Module : Modules) {
                if (auto Ret = std::any_cast<std::shared_ptr<T>>(&Module)) {
                    return **Ret;
                }
            }
            EGL3_LOG(LogLevel::Critical, "Could not find module");
        }

    private:
        std::vector<std::any> Modules;
    };
}
