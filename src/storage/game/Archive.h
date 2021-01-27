#pragma once

#include "../../utils/mmio/MmioFile.h"
#include "ArchiveRef.h"

#include "Header.h"
#include "Runlist.h"
#include "ManifestData.h"
#include "RunIndex.h"

#include <optional>

namespace EGL3::Storage::Game {
    enum class ArchiveMode : uint8_t {
        Create,
        Load,
        // Trying to write anything while opened in read mode will cause an exception!
        Read
    };

    // A full Fortnite (or maybe other game) installation
    // As a disclaimer, we assume that all structs are aligned to 8 bytes (in order for this to compile without UB)
    class Archive {
    public:
        Archive(const std::filesystem::path& Path, ArchiveMode) noexcept;

        bool IsValid() const;

        // Do not run these functions without checking for IsValid!
        const char* Get() const {
            return Backend->Get();
        }

        char* Get() {
            return Backend->Get();
        }

        const ArchiveRefConst<Header>& GetHeader() const {
            return Header;
        }

        const ArchiveRef<Game::Header>& GetHeader() {
            return Header;
        }

        const ArchiveRefConst<ManifestData>& GetManifestData() const {
            return ManifestData;
        }

        const ArchiveRef<Game::ManifestData>& GetManifestData() {
            return ManifestData;
        }

        template<RunlistId Id>
        const ArchiveRefConst<typename RunlistTraits<Id>::Runlist>& GetRunlist() const;

        template<RunlistId Id>
        const ArchiveRef<typename RunlistTraits<Id>::Runlist>& GetRunlist();

        template<>
        const ArchiveRefConst<RunlistTraits<RunlistId::File>::Runlist>& GetRunlist<RunlistId::File>() const {
            return RunlistFile;
        }

        template<>
        const ArchiveRef<RunlistTraits<RunlistId::File>::Runlist>& GetRunlist<RunlistId::File>() {
            return RunlistFile;
        }

        template<>
        const ArchiveRefConst<RunlistTraits<RunlistId::ChunkPart>::Runlist>& GetRunlist<RunlistId::ChunkPart>() const {
            return RunlistChunkPart;
        }

        template<>
        const ArchiveRef<RunlistTraits<RunlistId::ChunkPart>::Runlist>& GetRunlist<RunlistId::ChunkPart>() {
            return RunlistChunkPart;
        }

        template<>
        const ArchiveRefConst<RunlistTraits<RunlistId::ChunkInfo>::Runlist>& GetRunlist<RunlistId::ChunkInfo>() const {
            return RunlistChunkInfo;
        }

        template<>
        const ArchiveRef<RunlistTraits<RunlistId::ChunkInfo>::Runlist>& GetRunlist<RunlistId::ChunkInfo>() {
            return RunlistChunkInfo;
        }

        template<>
        const ArchiveRefConst<RunlistTraits<RunlistId::ChunkData>::Runlist>& GetRunlist<RunlistId::ChunkData>() const {
            return RunlistChunkData;
        }

        template<>
        const ArchiveRef<RunlistTraits<RunlistId::ChunkData>::Runlist>& GetRunlist<RunlistId::ChunkData>() {
            return RunlistChunkData;
        }

        const ArchiveRefConst<RunIndexTraits::RunIndex>& GetRunIndex() const {
            return RunIndex;
        }

        const ArchiveRef<RunIndexTraits::RunIndex>& GetRunIndex() {
            return RunIndex;
        }

        template<uint32_t MaxRunCount>
        size_t ReadRunlist(const Runlist<MaxRunCount>& Runlist, size_t Position, char* Dst, size_t DstSize) const;

        template<uint32_t MaxRunCount>
        void Resize(ArchiveRef<Runlist<MaxRunCount>>& Runlist, uint64_t NewSize);

        template<uint32_t MaxRunCount>
        void Reserve(ArchiveRef<Runlist<MaxRunCount>>& Runlist, uint64_t NewAllocatedSize);

    private:
        // at offset 0, size 256
        ArchiveRef<Header> Header;

        // at offset 256, size 1792
        ArchiveRef<ManifestData> ManifestData;

        // at offset 2048, size 14336
        ArchiveRef<RunlistTraits<RunlistId::File>::Runlist> RunlistFile;

        // at offset 16384, size 16384
        ArchiveRef<RunlistTraits<RunlistId::ChunkPart>::Runlist> RunlistChunkPart;

        // at offset 32768, size 16384
        ArchiveRef<RunlistTraits<RunlistId::ChunkInfo>::Runlist> RunlistChunkInfo;

        // at offset 49152, size 16384
        ArchiveRef<RunlistTraits<RunlistId::ChunkData>::Runlist> RunlistChunkData;

        // at offset 65536, size 65536
        ArchiveRef<RunIndexTraits::RunIndex> RunIndex;

        // at offset 131072 (sector 32)
        // technical start of file data and all

        std::optional<Utils::Mmio::MmioFile> Backend;
        bool Valid;
    };

    template size_t Archive::ReadRunlist<1789>(const Runlist<1789>&, size_t, char*, size_t) const;
    template size_t Archive::ReadRunlist<2045>(const Runlist<2045>&, size_t, char*, size_t) const;

    template void Archive::Resize<1789>(ArchiveRef<Runlist<1789>>&, uint64_t);
    template void Archive::Resize<2045>(ArchiveRef<Runlist<2045>>&, uint64_t);

    template void Archive::Reserve<1789>(ArchiveRef<Runlist<1789>>&, uint64_t);
    template void Archive::Reserve<2045>(ArchiveRef<Runlist<2045>>&, uint64_t);
}