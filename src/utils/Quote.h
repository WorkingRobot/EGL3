#pragma once

#include <string>

namespace EGL3::Utils {
    std::string Quote(const std::string& Input);

    std::string Unquote(const std::string& Input);
}
