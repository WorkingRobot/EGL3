#pragma once

#include <string>
#include <unordered_map>

namespace EGL3::Web::Epic::Friends {
    struct UserPresence {
        std::string Status = "online";
        std::string Activity;
        std::unordered_map<std::string, std::string> Properties;
        std::unordered_map<std::string, std::string> ConnectionProperties;
    };
}
