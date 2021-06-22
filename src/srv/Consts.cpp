#include "Consts.h"

namespace EGL3::Service {
    const char* GetServiceName()
    {
        return "EGL3 Launcher";
    }

    const char* GetServiceDescription()
    {
        return "Handles the mounting for .egia files on EGL3's behalf";
    }
}