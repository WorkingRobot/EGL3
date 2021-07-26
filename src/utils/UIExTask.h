#pragma once

#include "DataDispatcher.h"

namespace EGL3::Utils {
    // UI Exclusive Task
    // UI as in with a dispatcher at the end to show results to the user
    // Exclusive as in only one can exist at a time
    // Task as in it starts in another thread
    template<class T>
    struct UIExTask {
    public:
        UIExTask() :
            OnStart([]() -> T { return {}; }),
            Dispatcher([this](const T& Data) { OnDispatch(Data); Running.store(false); })
        {
            
        }

        bool IsRunning() const
        {
            return Future.wait_for(std::chrono::seconds(0)) == std::future_status::timeout;
        }

        bool Start()
        {
            { // Atomic CAS: if there is no update running, return, otherwise, set the flag and continue
                bool OldValue = Running.load();
                do {
                    if (OldValue) {
                        return false;
                    }
                } while (!Running.compare_exchange_weak(OldValue, true));
            }

            Future = std::async(std::launch::async, [this]() {
                Dispatcher.emit(OnStart());
            });

            return true;
        }

        Utils::Callback<T()> OnStart;
        Utils::Callback<void(const T&)> OnDispatch;

    private:
        std::atomic<bool> Running;
        std::shared_future<void> Future;
        DataDispatcher<T> Dispatcher;
    };
}