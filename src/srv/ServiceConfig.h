#pragma once

#include <string>

namespace EGL3::Service {
    int RunInstall();

    int RunQuery();

    int RunDescribe();

    int RunEnable();

    int RunDisable();

    int RunDelete();
}