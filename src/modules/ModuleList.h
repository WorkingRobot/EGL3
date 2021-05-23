#pragma once

#include "../storage/persistent/Store.h"
#include "../utils/Assert.h"
#include "../utils/GladeBuilder.h"
#include "BaseModule.h"

#include <gtkmm.h>

namespace EGL3::Modules {
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
            EGL3_LOG(LogLevel::Critical, "Could not find module");
        }

        template<typename T>
        T& GetModule() {
            for (auto& Module : Modules) {
                if (T* Ret = dynamic_cast<T*>(Module.get())) {
                    return *Ret;
                }
            }
            EGL3_LOG(LogLevel::Critical, "Could not find module");
        }

    private:
        void AddModules();

        template<typename T>
        void AddModule();

        Utils::GladeBuilder Builder;
        Storage::Persistent::Store Storage;
        std::vector<std::unique_ptr<BaseModule>> Modules;
    };
}
