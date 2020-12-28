#include "RunningFunctionGuard.h"

namespace EGL3::Web {
    RunningFunctionGuard::RunningFunctionGuard(RunningFunctionLock& Lock) : Lock(Lock) {
        {
            std::lock_guard lock(Lock.Mutex);
            Lock.RunningCount++;
        }
        Lock.CV.notify_all();
    }

    RunningFunctionGuard::~RunningFunctionGuard() {
        {
            std::lock_guard lock(Lock.Mutex);
            Lock.RunningCount--;
        }
        Lock.CV.notify_all();
    }
}
