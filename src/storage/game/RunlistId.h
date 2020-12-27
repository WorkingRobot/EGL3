#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
    enum RunlistId : uint32_t {
        RUNLIST_ID_FILE			= 0xF114D163,
        RUNLIST_ID_CHUNK_PART	= 0x1EE4171A,
        RUNLIST_ID_CHUNK_INFO	= 0x2E0B1019,
        RUNLIST_ID_CHUNK_DATA	= 0x52863CFE,
    };
}