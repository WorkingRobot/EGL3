#include "ChunkHeader.h"

namespace EGL3::Web::Epic::BPS {
    // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/BuildPatchServices/Private/Data/ChunkData.cpp#L157
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, ChunkHeader& Val) {
        auto StartPos = Stream.tell();
        
        Stream >> Val.Magic;
        if (Val.Magic != ChunkHeader::ExpectedMagic) {
            return Stream;
        }
        uint32_t Version;
        Stream >> Version;
        Val.Version = (ChunkVersion)Version;
        Stream >> Val.HeaderSize;
        Stream >> Val.DataSizeCompressed;
        Stream >> Val.Guid;
        Stream >> Val.RollingHash;
        uint8_t StoredAs;
        Stream >> StoredAs;
        Val.StoredAs = (ChunkStorageFlags)StoredAs;

        if (Val.Version >= ChunkVersion::StoresShaAndHashType) {
            Stream >> Val.SHAHash;
            uint8_t HashType;
            Stream >> HashType;
            Val.HashType = (ChunkHashFlags)HashType;
        }

        if (Val.Version >= ChunkVersion::StoresDataSizeUncompressed) {
            Stream >> Val.DataSizeUncompressed;
        }
        else {
            Val.DataSizeUncompressed = 1 << 20;
        }

        Stream.seek(StartPos + Val.HeaderSize, Utils::Streams::Stream::Beg);
        return Stream;
    }

    bool ChunkHeader::IsCompressed() const {
        return (uint8_t)StoredAs & (uint8_t)ChunkStorageFlags::Compressed;
    }

    bool ChunkHeader::IsEncrypted() const {
        return (uint8_t)StoredAs & (uint8_t)ChunkStorageFlags::Encrypted;
    }

    bool ChunkHeader::UsesRollingHash() const
    {
        return (uint8_t)HashType & (uint8_t)ChunkHashFlags::RollingPoly64;
    }

    bool ChunkHeader::UsesSha1() const
    {
        return (uint8_t)HashType & (uint8_t)ChunkHashFlags::Sha1;
    }
}