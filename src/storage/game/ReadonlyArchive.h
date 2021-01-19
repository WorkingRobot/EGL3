#pragma once

#include "../../utils/mmio/MmioFile.h"
#include "GameId.h"
#include "Header.h"
#include "Runlist.h"
#include "ManifestData.h"
#include "RunIndex.h"

#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace EGL3::Storage::Game {
    // A full Fortnite (or maybe other game) installation
    // As a disclaimer, we assume that all structs are aligned to 8 bytes (in order for this to compile without UB)
    class ReadonlyArchive {
    public:
        const Utils::Mmio::MmioReadonlyFile Backend;

        // at offset 0
        const Header* Header;

        // at offset 256
        const Runlist* FileRunlist;

        // at offset 512
        const Runlist* ChunkPartRunlist;

        // at offset 768
        const Runlist* ChunkInfoRunlist;

        // at offset 1024
        const Runlist* ChunkDataRunlist;

        // at offset 2048
        const ManifestData* ManifestData;

        // at offset 4096
        const RunIndex* RunIndex;

        // at offset 8192 (sector 16)
        // technical start of file data and all

        ReadonlyArchive(const fs::path& Path);

        ~ReadonlyArchive();
    };
}