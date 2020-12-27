#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
    struct RunlistElement {
        uint32_t SectorOffset;			// 1 sector = 512 bytes, this is the offset in sectors from the beginning of the install archive
        uint32_t SectorSize;			// Number of 512 byte sectors this run consists of
    };
}