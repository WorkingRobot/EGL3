#pragma once

#include "ChunkInfo.h"
#include "UEStream.h"

namespace EGL3::Web::Epic::BPS {
    enum class ChunkDataListVersion : uint8_t {
        Original,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    struct ChunkDataList {
        std::vector<ChunkInfo> ChunkList;

        friend UEStream& operator>>(UEStream& Stream, ChunkDataList& Val);
    };
}