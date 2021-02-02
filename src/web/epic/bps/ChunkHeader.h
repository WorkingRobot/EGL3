#pragma once

#include "../../../utils/streams/Stream.h"
#include "../../../utils/Guid.h"
#include "ChunkHashFlags.h"
#include "ChunkStorageFlags.h"
#include "ChunkVersion.h"

namespace EGL3::Web::Epic::BPS {
    struct ChunkHeader {
        uint32_t Magic;
        ChunkVersion Version;
        uint32_t HeaderSize;
        Utils::Guid Guid;
        uint32_t DataSizeCompressed;
        uint32_t DataSizeUncompressed;
        ChunkStorageFlags StoredAs;
        ChunkHashFlags HashType;
        uint64_t RollingHash;
        char SHAHash[20];

        static constexpr uint32_t ExpectedMagic = 0xB1FE3AA2;

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, ChunkHeader& Val);

        bool IsCompressed() const;

        bool IsEncrypted() const;

        bool UsesRollingHash() const;

        bool UsesSha1() const;
    };
}