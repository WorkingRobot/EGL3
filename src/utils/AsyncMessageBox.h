#pragma once

#ifdef SERVICE_NAME
#error AsyncMessageBox cannot be used as a service.
#endif

#include <stdint.h>

namespace EGL3::Utils {
    void AsyncMessageBox(const char Text[2048], const char Title[256], uint32_t Type);
}