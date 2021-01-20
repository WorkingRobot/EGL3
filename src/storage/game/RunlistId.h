#pragma once

#include "../../utils/Crc32.h"

namespace EGL3::Storage::Game {
    enum class RunlistId : uint8_t {
        File,
        ChunkPart,
        ChunkInfo,
        ChunkData,
    };
}