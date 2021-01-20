#pragma once

#include <stdint.h>

namespace EGL3::Storage::Models {
    struct UpdateStatsInfo {
        // ForRemoval doesn't count
        // This is how many chunks will be in use at the end
        uint64_t ChunkCountTotal;

        uint64_t ChunkCountForRemoval;
        uint64_t ChunkCountRemoving;
        uint64_t ChunkCountRemoved;

        uint64_t ChunkCountForUpdate;
        uint64_t ChunkCountDownloading;
        uint64_t ChunkCountDecompressing;
        uint64_t ChunkCountWriting;
        uint64_t ChunkCountUpdated;

        uint64_t NetworkBytesUsed;
        uint64_t NetworkBytesTotal;

        uint64_t DiskBytesUsed;
        uint64_t DiskBytesTotal;
    };
}