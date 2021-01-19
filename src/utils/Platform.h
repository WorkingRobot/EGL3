#pragma once

#include <string>

namespace EGL3::Utils::Platform {
    constexpr const char* GetOSName() {
        return "Windows";
    }

    const std::string& GetOSVersion();
}