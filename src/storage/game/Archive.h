#pragma once

#include "../../utils/mmio/MmioFile.h"
#include "GameId.h"
#include "Header.h"
#include "ManifestData.h"
#include "RunIndex.h"
#include "Runlist.h"

#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

namespace EGL3::Storage::Game {
    namespace Detail {
        struct ECreate {
            explicit ECreate() = default;
        };

        struct ELoad {
            explicit ELoad() = default;
        };
    }

    inline constexpr Detail::ECreate ArchiveOptionCreate{};
    inline constexpr Detail::ELoad ArchiveOptionLoad{};

    // A full Fortnite (or maybe other game) installation
    // As a disclaimer, we assume that all structs are aligned to 8 bytes (in order for this to compile without UB)
    class Archive {
    public:
        Archive(const fs::path& Path, Detail::ECreate) noexcept;

        Archive(const fs::path& Path, Detail::ELoad) noexcept;

        ~Archive();

        bool IsValid() const;

        // Do not run these functions without checking for IsValid!
        std::string GetGame() const;

        std::string GetVersionLong() const;

        std::string GetVersionHR() const;

        uint64_t GetVersionNum() const;

        GameId GetGameId() const;

    private:
        void Resize(Runlist* Runlist, int64_t NewSize);

        // All offsets (except for the header) are suggestions. They can be moved or changed based on the header
        // However, that should be avoided, as errors may occur, and those offsets are not tested
        // I may change them to be hardcoded

        // at offset 0, size 256
        Header* Header;

        // at offset 256, size 1792
        ManifestData* ManifestData;

        // at offset 2048, size 14336
        Runlist* FileRunlist;

        // at offset 16384, size 16384
        Runlist* ChunkPartRunlist;

        // at offset 32768, size 16384
        Runlist* ChunkInfoRunlist;

        // at offset 49152, size 16384
        Runlist* ChunkDataRunlist;

        // at offset 65536, size 65536
        RunIndex* RunIndex;

        // at offset 131072 (sector 256)
        // technical start of file data and all

        std::optional<Utils::Mmio::MmioFile> Backend;

        bool Valid;

        friend class RunlistStream;
    };
}