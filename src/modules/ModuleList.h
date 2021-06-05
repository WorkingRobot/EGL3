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

        const Utils::GladeBuilder& GetBuilder() const;

        template<class T>
        T& GetWidget(const char* Name) const {
            return Builder.GetWidget<T>(Name);
        }

        const Storage::Persistent::Store& GetStorage() const;

        Storage::Persistent::Store& GetStorage();

        template<uint32_t Constant, class T>
        T& Get(const Storage::Persistent::KeyType<Constant, T>& Key) {
            return Storage.Get(Key);
        }

        template<typename T>
        const T& GetModule() const {
            for (auto& Module : Modules) {
                if (T* Ret = dynamic_cast<T*>(Module.get())) {
                    return *Ret;
                }
            }
            EGL3_ABORTF("Could not find module {}", Detail::module_name_v<T>);
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

    private:
        void AddModulesCore();

        void AddModulesLoggedIn();

        template<typename T>
        void AddModule();

        Utils::GladeBuilder Builder;
        Storage::Persistent::Store Storage;
        std::vector<std::unique_ptr<BaseModule>> Modules;
        Glib::Dispatcher LoggedInDispatcher;
    };
}
