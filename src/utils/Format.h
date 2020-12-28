#pragma once

#include <memory>
#include <string>

namespace EGL3::Utils {
    static std::string Format(const char* Input) {
        return Input;
    }

    template<typename... Args>
    static std::string Format(const char* Input, Args&&... FormatArgs) {
        auto BufSize = snprintf(nullptr, 0, Input, FormatArgs...) + 1;
        auto Buf = std::make_unique<char[]>(BufSize);
        snprintf(Buf.get(), BufSize, Input, FormatArgs...);
        return std::string(Buf.get(), BufSize - 1);
    }
}
