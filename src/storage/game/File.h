#pragma once

namespace EGL3::Storage::Game {
    struct File {
        char Filename[220];             // Engine/Binaries/ThirdParty/CEF3/Win64/chrome_elf.dll
        char SHA[20];                   // SHA1 hash of the entire file
        uint64_t FileSize;              // Size of the file (calculated from chunk part data)
        uint32_t ChunkPartDataStartIdx; // Index to the first chunk part (in chunk part data runlist)
        uint32_t ChunkPartDataSize;     // Number of chunk parts
    };

    static_assert(sizeof(File) == 256);
}