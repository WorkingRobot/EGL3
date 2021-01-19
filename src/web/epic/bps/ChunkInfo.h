#pragma once

#include "Guid.h"

#include <stdint.h>

namespace EGL3::Web::Epic::BPS {
    struct ChunkInfo {
        Guid Guid;
        uint64_t Hash;
        char SHAHash[20];
        uint8_t GroupNumber;
        uint32_t WindowSize;
        int64_t FileSize;
    };
}