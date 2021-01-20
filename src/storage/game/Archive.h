#pragma once

#include "../../utils/mmio/MmioFile.h"
#include "ArchiveReadonly.h"

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
    class Archive : public ArchiveReadonly {
    public:
        Archive(const std::filesystem::path& Path, Detail::ECreate) noexcept;

        Archive(const std::filesystem::path& Path, Detail::ELoad) noexcept;

        char* Get();

        template<uint32_t MaxRunCount>
        void Resize(Runlist<MaxRunCount>& Runlist, uint64_t NewSize);

    protected:
        Utils::Mmio::MmioFile& GetBackend();

        // Do not run these functions without checking for IsValid!
        Header& GetHeader();

        ManifestData& GetManifestData();

        RunlistFile& GetRunlistFile();

        RunlistChunk& GetRunlistChunkPart();

        RunlistChunk& GetRunlistChunkInfo();

        RunlistChunk& GetRunlistChunkData();

        RunIndexBase& GetRunIndex();
    };

    template void Archive::Resize(Archive::RunlistFile& Runlist, uint64_t NewSize);

    template void Archive::Resize(Archive::RunlistChunk& Runlist, uint64_t NewSize);
}