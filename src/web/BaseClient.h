#pragma once

#include "RunningFunctionLock.h"

#include <atomic>

namespace EGL3::Web {
    class BaseClient {
    protected:
        // Always call this in the destructor before running any "kill token" style tasks!
        void EnsureCallCompletion() {
            Cancelled = true;

            // Prevents the client functions from running past the lifetime of itself
            Lock.EnsureCallCompletion();
        }

        ~BaseClient() {
            EnsureCallCompletion();
        }

        __forceinline const std::atomic_bool& GetCancelled() const {
            return Cancelled;
        }

    protected:
        RunningFunctionLock Lock;

    private:
        std::atomic_bool Cancelled;
    };
}
