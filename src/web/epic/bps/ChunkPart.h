#pragma once

#include "UEStream.h"

namespace EGL3::Web::Epic::BPS {
    struct ChunkPart {
        Utils::Guid Guid;
        uint32_t Offset;
        uint32_t Size;

        friend UEStream& operator>>(UEStream& Stream, ChunkPart& Val);
    };
}