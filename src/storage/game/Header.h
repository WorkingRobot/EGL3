#pragma once

#include "ArchiveVersion.h"
#include "GameId.h"

namespace EGL3::Storage::Game {
    // Strings are not required to be 0 terminated!
    struct Header {
        uint32_t Magic;                     // 'EGIA' aka 0x41494745
        ArchiveVersion ArchiveVersion;      // Version of the install archive, currently 0
        uint16_t HeaderSize;                // Size of the header
        char Game[40];                      // Game name - example: Fortnite
        char VersionStringLong[64];         // Example: ++Fortnite+Release-13.30-CL-13884634-Windows
        char VersionStringHR[20];           // Human readable version - example: 13.30
        GameId GameNumeric;                 // Should be some sort of enum (GameId)
        uint64_t VersionNumeric;            // Game specific, but a new update will always have a higher number (e.g. Fortnite's P4 CL)

        static constexpr uint32_t ExpectedMagic = 0x41494745;

        static constexpr uint64_t ManifestDataOffset = 256;
        static constexpr uint64_t FileRunlistOffset = 2048;
        static constexpr uint64_t ChunkPartRunlistOffset = 16384;
        static constexpr uint64_t ChunkInfoRunlistOffset = 32768;
        static constexpr uint64_t ChunkDataRunlistOffset = 49152;
        static constexpr uint64_t RunIndexOffset = 65536;

        static constexpr uint64_t SectorSize = 512;
        static constexpr uint64_t FileDataOffset = 131072;

        static constexpr uint64_t FileDataSectorOffset = FileDataOffset / SectorSize;
    };
}