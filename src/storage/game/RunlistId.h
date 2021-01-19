#pragma once

#include "../../utils/Crc32.h"

namespace EGL3::Storage::Game {
    enum class RunlistId : uint32_t {
#define KEY(Name) Name = ~Utils::Crc32(#Name),

        KEY(File)
        KEY(ChunkPart)
        KEY(ChunkInfo)
        KEY(ChunkData)

#undef KEY
    };
}