#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace EGL3::Utils {
    static void FromBlob(const char* Blob, char* Output, uint32_t OutputSize) {
        char ConvBuf[4];
        ConvBuf[3] = '\0';
        for (uint32_t i = 0; i < OutputSize; ++i) {
            ConvBuf[0] = Blob[i * 3];
            ConvBuf[1] = Blob[i * 3 + 1];
            ConvBuf[2] = Blob[i * 3 + 2];
            Output[i] = atoi(ConvBuf);
        }
    }
}