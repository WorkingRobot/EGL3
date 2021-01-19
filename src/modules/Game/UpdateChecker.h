#pragma once

#include "../../storage/persistent/Store.h"
#include "../../utils/Callback.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../Authorization.h"

#include <future>
#include <sigc++/sigc++.h>

namespace EGL3::Modules::Game {
    class UpdateCheckerModule : public BaseModule {
    public:
        UpdateCheckerModule(ModuleList& Modules, Storage::Persistent::Store& Storage);

        ~UpdateCheckerModule();

        Utils::Callback<uint32_t()> RequestVersionNum;

        sigc::signal<void()> UpdateAvailable;

    private:
        void CheckForUpdate();

        void BackgroundTask();

        Storage::Persistent::Store& Storage;
        Modules::AuthorizationModule& Auth;

        std::mutex Mutex;
        std::condition_variable CV;
        bool Cancelled;
        std::future<void> Future;
    };
}