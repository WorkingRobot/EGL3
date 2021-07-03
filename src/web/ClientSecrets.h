#pragma once

#include "Http.h"

namespace EGL3::Web {
    static const cpr::Authentication AuthClientLauncher{ "34a02cf8f4414e29b15921876da36f9a", "daafbccc737745039dffe53d94fc76cf" };
    static const cpr::Authentication AuthClientAndroid{ "3f69e56c7649492c8cc29f1af08a8a12", "b51ee9cb12234f50a69efa67ef53812e" };
    static const cpr::Authentication AuthClientPC{ "ec684b8c687f479fadea3cb2ad83f5c6", "e1f31c211f28413186262d37a13fc84d" };

    static constexpr const char* ClientIdEOS = "xyza7891kXAfZljmmXBQvWTfhWa8bT7e";
    static const std::string DeploymentIdEOS = "83a532913dae4f459ba8a0b5d11bb279";
    static const cpr::Authentication AuthClientEOS{ ClientIdEOS, "PpkHY9t/WRLW28r8eMSwNk5xuqDIv+X7KGZ4EXPGuP4" };
}