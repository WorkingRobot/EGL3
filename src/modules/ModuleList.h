#pragma once

#include "../storage/persistent/Store.h"
#include "../utils/GladeBuilder.h"
#include "../utils/Log.h"
#include "../utils/type_name.h"
#include "BaseModule.h"

namespace EGL3::Modules {
    namespace Detail {
        template<typename T, std::enable_if_t<type_name_v<T>.starts_with("class EGL3::Modules::") && std::is_base_of_v<BaseModule, T>, bool> = true>
        inline constexpr std::string_view module_name_v = type_name_v<T>.substr(sizeof("class EGL3::Modules::") - 1);
    }

    class ModuleList {
    public:
        ModuleList(const std::filesystem::path& BuilderPath, const std::filesystem::path& StoragePath);

        ~ModuleList();

        template<class T>
        T& GetWidget(const char* Name) const {
            return Builder.GetWidget<T>(Name);
        }

        template<class Setting>
        auto Get() {
            return Storage.Get<Setting>();
        }

        template<typename T>
        T& GetModule() {
            for (auto& Module : Modules) {
                if (T* Ret = dynamic_cast<T*>(Module.get())) {
                    return *Ret;
                }
            }
            EGL3_ABORTF("Could not find module {}", Detail::module_name_v<T>);
        }

        bool DisplayConfirmation(const Glib::ustring& Message, const Glib::ustring& Title, bool UseMarkup = false) const;

        bool DisplayConfirmation(const Glib::ustring& Message, const Glib::ustring& SecondaryMessage, const Glib::ustring& Title, bool UseMarkup = false) const;

        void DisplayError(const Glib::ustring& Message, const Glib::ustring& Title, bool UseMarkup = false) const;

        void DisplayError(const Glib::ustring& Message, const Glib::ustring& SecondaryMessage, const Glib::ustring& Title, bool UseMarkup = false) const;

    private:
        void AddModulesCore();

        void AddModulesLoggedIn();

        void RemoveModulesLoggedIn();

        template<typename T>
        void AddModule();

        Utils::GladeBuilder Builder;
        Storage::Persistent::Store Storage;
        std::vector<std::unique_ptr<BaseModule>> Modules;
        uint32_t AuthedModulesIdx;
        Glib::Dispatcher LoggedInDispatcher;
    };
}
