#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
    struct RunlistElement {
        uint32_t SectorOffset;          // This is the offset in sectors from the beginning of the install archive
        uint32_t SectorCount;           // Number of sectors this run consists of
    };

    static_assert(sizeof(RunlistElement) == 8);
}