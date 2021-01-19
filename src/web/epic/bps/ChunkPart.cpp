#include "ChunkPart.h"

namespace EGL3::Web::Epic::BPS {
    UEStream& operator>>(UEStream& Stream, ChunkPart& Val) {
        auto StartPos = Stream.tell();

        uint32_t DataSize;
        Stream >> DataSize;

        Stream >> Val.Guid;
        Stream >> Val.Offset;
        Stream >> Val.Size;

        Stream.seek(StartPos + DataSize, UEStream::Beg);
        return Stream;
    }
}