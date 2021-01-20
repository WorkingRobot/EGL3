#pragma once

#include "../../utils/mmio/MmioFileReadonly.h"
#include "Header.h"
#include "Runlist.h"
#include "ManifestData.h"
#include "RunIndex.h"

#include <filesystem>
#include <optional>

namespace EGL3::Storage::Game {
    // A full Fortnite (or maybe other game) installation
    // As a disclaimer, we assume that all structs are aligned to 8 bytes (in order for this to compile without UB)
    class ArchiveReadonly {
    public:
        ArchiveReadonly(const std::filesystem::path& Path) noexcept;

        bool IsValid() const;

        const char* Get() const;

    protected:
        ArchiveReadonly();

        using RunlistFile = Runlist<1790>;
        using RunlistChunk = Runlist<2046>;
        using RunIndexBase = RunIndex<8191>;

        static_assert(sizeof(RunlistFile) == 14336);
        static_assert(sizeof(RunlistChunk) == 16384);
        static_assert(sizeof(RunIndexBase) == 65536);

        const Utils::Mmio::MmioFileReadonly& GetBackend() const;

        // Do not run these functions without checking for IsValid!
        const Header& GetHeader() const;

        const ManifestData& GetManifestData() const;

        const RunlistFile& GetRunlistFile() const;

        const RunlistChunk& GetRunlistChunkPart() const;

        const RunlistChunk& GetRunlistChunkInfo() const;

        const RunlistChunk& GetRunlistChunkData() const;

        const RunIndexBase& GetRunIndex() const;

        // at offset 0, size 256
        // Header

        // at offset 256, size 1792
        // ManifestData

        // at offset 2048, size 14336
        // RunlistFile

        // at offset 16384, size 16384
        // RunlistChunkPart

        // at offset 32768, size 16384
        // RunlistChunkInfo

        // at offset 49152, size 16384
        // RunlistChunkData

        // at offset 65536, size 65536
        // RunlistIndex

        // at offset 131072 (sector 32)
        // technical start of file data and all

        std::unique_ptr<Utils::Mmio::MmioFileReadonly> Backend;

        bool Valid;
    };
}