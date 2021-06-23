#pragma once

#include "../storage/game/ArchiveList.h"
#include "DriveCounter.h"
#include "MountedDisk.h"

namespace EGL3::Service {
    class MountedArchive {
    public:
        MountedArchive(const std::filesystem::path& Path, Storage::Game::Archive&& Archive);

        ~MountedArchive();
        
        char GetDriveLetter() const;

        std::filesystem::path GetArchivePath() const;

        using Stats = DriveCounter::Data;

        Stats QueryStats() const;

    private:
        struct Lists {
            const Storage::Game::ArchiveList<Storage::Game::RunlistId::File> Files;
            const Storage::Game::ArchiveList<Storage::Game::RunlistId::ChunkPart> ChunkParts;
            const Storage::Game::ArchiveList<Storage::Game::RunlistId::ChunkInfo> ChunkInfos;
            const Storage::Game::ArchiveList<Storage::Game::RunlistId::ChunkData> ChunkDatas;

            Lists(Storage::Game::Archive& Archive) :
                Files(Archive),
                ChunkParts(Archive),
                ChunkInfos(Archive),
                ChunkDatas(Archive)
            {

            }
        };

        struct SectionContext {
            // Idx is LCN, stores a pair of (PartIdx, Offset)
            const std::vector<std::pair<uint32_t, uint32_t>> LUT;
            // One for every corresponding ChunkPart, stored as (Itr [with its offset already added], Size)
            const std::vector<std::pair<Storage::Game::ArchiveListIteratorReadonly<Storage::Game::RunlistId::ChunkData>, uint32_t>> Parts;

            SectionContext(std::vector<std::pair<uint32_t, uint32_t>>&& LUT, std::vector<std::pair<Storage::Game::ArchiveListIteratorReadonly<Storage::Game::RunlistId::ChunkData>, uint32_t>>&& Parts);
        };

        struct SectionLUT {
            // One per file
            std::vector<SectionContext> Contexts;

            SectionLUT(const Lists& ArchiveLists);
        };

        std::vector<MountedFile> GetMountedFiles();

        static void HandleCluster(void* CtxPtr, uint32_t LCN, uint32_t Count, uint8_t* Buffer) noexcept;

        Storage::Game::Archive Archive;
        const Lists ArchiveLists;
        const SectionLUT LUT;
        MountedDisk Disk;
        mutable DriveCounter Counter;
        mutable char DriveLetter;
    };
}