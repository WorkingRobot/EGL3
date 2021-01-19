#pragma once

#include "RunIndexElement.h"

namespace EGL3::Storage::Game {
    struct RunIndex {
        uint32_t NextAvailableSector;   // Next available free sector
        uint32_t ElementCount;          // Number of elements in element array
        RunIndexElement Elements[];     // Accumulation of all run indexes in all runs
    };
}