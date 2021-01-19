#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
    struct File {
        char Filename[236];             // Engine/Binaries/ThirdParty/CEF3/Win64/chrome_elf.dll
        char SHA[20];                   // SHA1 hash of the entire file
        int64_t ChunkPartDataOffset;    // Offset to the first chunk part (in chunk part data runlist)
        int64_t ChunkPartDataSize;      // Number of chunk part data in bytes, 8 byte aligned (it always is atm)
        int64_t FileSize;               // Size of the file (calculated from chunk part data)
    };
}