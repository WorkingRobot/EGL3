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
        void HandleFileCluster(void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) const;

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

        struct SectionPart {
            uint64_t Data;

            SectionPart() :
                Data(0)
            {

            }

            SectionPart(uint64_t Ptr, uint16_t Size) :
                Data(uint64_t(Size) << 48 | Ptr)
            {

            }

            bool IsValid() const
            {
                return Data;
            }

            uint64_t GetPtr() const
            {
                return Data & (1llu << 48) - 1;
            }

            uint16_t GetSize() const
            {
                return Data >> 48;
            }
        };

        Storage::Game::Archive Archive;
        Lists ArchiveLists;
        MountedDisk Disk;
        std::vector<SectionPart> SectionParts;
        std::vector<std::vector<uint32_t>> SectionLUT;
        mutable DriveCounter Counter;
        mutable char DriveLetter;
    };
}