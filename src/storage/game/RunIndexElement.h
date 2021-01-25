#pragma once

#include "RunlistId.h"

namespace EGL3::Storage::Game {
#pragma pack(push, 2)
    struct RunIndexElement {
        uint32_t SectorCount;           // The number of sectors this element has
        RunlistId Id;                   // The id of the runlist that this element is allocated to
    };
#pragma pack(pop)
    static_assert(sizeof(RunIndexElement) == 6);
}