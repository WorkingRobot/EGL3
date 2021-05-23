#pragma once

#include "../storage/persistent/Store.h"
#include "../utils/Assert.h"
#include "../utils/GladeBuilder.h"

#include <any>
#include <gtkmm.h>

namespace EGL3::Modules {
    class ModuleList {
    public:
        ModuleList(const std::filesystem::path& BuilderPath, const std::filesystem::path& StoragePath);

        ~ModuleList();

        const Utils::GladeBuilder& GetBuilder() const;

        const Storage::Persistent::Store& GetStorage() const;

        Storage::Persistent::Store& GetStorage();

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
        void AddModules();

        template<typename T, typename... Args>
        void AddModule(Args&&... ModuleArgs);

        Utils::GladeBuilder Builder;
        Storage::Persistent::Store Storage;
        std::vector<std::any> Modules;
    };
}
