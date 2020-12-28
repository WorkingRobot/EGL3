#pragma once

#include <mutex>
#include <condition_variable>

namespace EGL3::Web {
    class RunningFunctionLock {
        std::mutex Mutex;
        std::condition_variable CV;
        int RunningCount = 0;

        friend class RunningFunctionGuard;

    public:
        void EnsureCallCompletion() {
            std::unique_lock Lock(Mutex);
            CV.wait(Lock, [this] { return RunningCount <= 0; });
        }
    };
}
