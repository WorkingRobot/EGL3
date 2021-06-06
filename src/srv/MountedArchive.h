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

        struct SectionPart {
            uint64_t Data;

            SectionPart() noexcept :
                Data(0)
            {

            }

            SectionPart(uint64_t Ptr, uint16_t Size) noexcept :
                Data(uint64_t(Size) << 48 | Ptr)
            {

            }

            bool IsValid() const noexcept
            {
                return Data;
            }

            uint64_t GetPtr() const noexcept
            {
                return Data & (1llu << 48) - 1;
            }

            uint16_t GetSize() const noexcept
            {
                return Data >> 48;
            }
        };

        struct SectionLUT;

        struct SectionContext {
            const std::vector<uint32_t>& const LUT;
            const std::vector<SectionPart>& const Parts;
            const Storage::Game::ArchiveListIteratorReadonly<Storage::Game::RunlistId::ChunkData> DataBegin;

            SectionContext(const std::vector<uint32_t>& const LUT, const std::vector<SectionPart>& const Parts, const Storage::Game::ArchiveListIteratorReadonly<Storage::Game::RunlistId::ChunkData> DataBegin);
        };

        struct SectionLUT {
            std::vector<SectionPart> Parts;
            std::vector<std::vector<uint32_t>> LUT;
            std::vector<SectionContext> Contexts;

            SectionLUT(const Lists& ArchiveLists);
        };

        std::vector<MountedFile> GetMountedFiles();

        static void HandleCluster(void* CtxPtr, uint64_t LCN, uint8_t Buffer[4096]) noexcept;

        Storage::Game::Archive Archive;
        const Lists ArchiveLists;
        const SectionLUT LUT;
        MountedDisk Disk;
        mutable DriveCounter Counter;
        mutable char DriveLetter;
    };
}