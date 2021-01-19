#pragma once

#include <stdint.h>

namespace EGL3::Utils {
    void AsyncMessageBox(const char Text[2048], const char Title[256], uint32_t Type);
}