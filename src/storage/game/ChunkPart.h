#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
    struct ChunkPart {
        uint32_t ChunkIdx;              // Index of the chunk in the ChunkInfo list
        uint32_t Reserved;
        uint32_t Offset;                // Offset of the chunk data to start reading from
        uint32_t Size;                  // Number of bytes of the chunk data to read from
    };

    static_assert(sizeof(ChunkPart) == 16);
}