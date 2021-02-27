#pragma once

#include "../storage/models/MountedDisk.h"
#include "../storage/game/ArchiveList.h"
#include "DriveCounter.h"

namespace EGL3::Service {
    class MountedArchive {
    public:
        MountedArchive(const std::filesystem::path& Path);

        ~MountedArchive();

        bool OpenArchive();

        bool ReadArchive();

        bool InitializeDisk();

        bool CreateLUT();

        bool CreateDisk();

        bool MountDisk();
        
        char QueryDriveLetter() const;

        const std::filesystem::path& QueryPath() const;

        const Storage::Game::ArchiveRefConst<Storage::Game::Header>& QueryHeader() const;

        const Storage::Game::ArchiveRefConst<Storage::Game::ManifestData>& QueryMeta() const;

        using Stats = DriveCounter::Data;

        [[nodiscard]]
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
#pragma pack(push, 4)
        struct SectionPart {
            uint64_t Ptr;
            uint32_t Size;
        };

        struct FileSection {
            SectionPart Sections[3];

            FileSection() :
                Sections{}
            {

            }

            uint32_t TotalSize() const {
                return Sections[0].Size + Sections[1].Size + Sections[2].Size;
            }
        };
#pragma pack(pop)

        std::filesystem::path Path;
        std::optional<Storage::Game::Archive> Archive;
        std::optional<Storage::Models::MountedDisk> Disk;
        std::optional<Lists> ArchiveLists;
        std::vector<std::vector<FileSection>> SectionLUT;
        mutable DriveCounter Counter;
        mutable char DriveLetter;
    };
}