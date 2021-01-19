#include "ChunkDataList.h"

namespace EGL3::Web::Epic::BPS {
    UEStream& operator>>(UEStream& Stream, ChunkDataList& Val) {
        auto StartPos = Stream.tell();

        uint32_t DataSize;
        ChunkDataListVersion DataVersion;
        int32_t ElementCount;
        {
            Stream >> DataSize;
            uint8_t DataVersionRaw;
            Stream >> DataVersionRaw;
            DataVersion = (ChunkDataListVersion)DataVersionRaw;
            Stream >> ElementCount;
        }

        Val.ChunkList.resize(ElementCount);

        if (DataVersion >= ChunkDataListVersion::Original) {
            for (auto& Chunk : Val.ChunkList) { Stream >> Chunk.Guid; }
            for (auto& Chunk : Val.ChunkList) { Stream >> Chunk.Hash; }
            for (auto& Chunk : Val.ChunkList) { Stream >> Chunk.SHAHash; }
            for (auto& Chunk : Val.ChunkList) { Stream >> Chunk.GroupNumber; }
            for (auto& Chunk : Val.ChunkList) { Stream >> Chunk.WindowSize; }
            for (auto& Chunk : Val.ChunkList) { Stream >> Chunk.FileSize; }
        }

        Stream.seek(StartPos + DataSize, UEStream::Beg);
        return Stream;
    }
}