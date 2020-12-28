#pragma once

#include "RunningFunctionLock.h"

namespace EGL3::Web {
    class RunningFunctionGuard {
    public:
        RunningFunctionGuard(RunningFunctionLock& Lock);

        ~RunningFunctionGuard();

    private:
        RunningFunctionLock& Lock;
    };
}
