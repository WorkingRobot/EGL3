#pragma once

#include "../../utils/Guid.h"

namespace EGL3::Storage::Game {
    struct ChunkInfo {
        Utils::Guid Guid;               // GUID of the chunk, used in EGL's manifests, unique
        char SHA[20];                   // SHA hash of the chunk to check against
        uint32_t UncompressedSize;      // Uncompressed, total size that the chunk can provide (like window size in BPS)
        uint32_t CompressedSize;        // Stored size of the chunk in the data runlist (this can mean compressed size, or uncompressed size if the chunk is uncompressed)
        uint32_t DataSector;            // Sector of where the chunk data starts (in the runlist, not in the archive itself)
        uint64_t Hash;                  // Hash of the chunk, used in EGL's manifests, used to get chunk url
        uint16_t Flags;                 // Flags of the chunk, marks multiple things, like compression method
        uint8_t DataGroup;              // Data group of the chunk, used in EGL's manifests, used to get chunk url
    };

    static_assert(sizeof(ChunkInfo) == 64);
}