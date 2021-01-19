#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
    enum class ArchiveVersion : uint16_t {
        Initial = 0,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };
}