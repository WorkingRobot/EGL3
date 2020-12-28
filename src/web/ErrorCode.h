#pragma once

#include <stdint.h>

namespace EGL3::Web {
    enum class ErrorCode : uint8_t {
        Success,
        Cancelled,
        InvalidToken,
        Unsuccessful,
        NotJson,
        BadJson
    };
}
