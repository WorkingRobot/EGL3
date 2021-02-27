#include "MountedArchive.h"

#include "../utils/Align.h"
#include "../utils/Random.h"

namespace EGL3::Service {
    using namespace Storage;
    using namespace Storage::Models;

    MountedArchive::MountedArchive(const std::filesystem::path& Path) :
        Path(Path),
        DriveLetter(0)
    {
    }

    MountedArchive::~MountedArchive()
    {
    }

    bool MountedArchive::OpenArchive()
    {
        Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Read);
        if (!Archive.IsValid()) {
            return false;
        }

        this->Archive.emplace(std::move(Archive));
        return true;
    }

    bool MountedArchive::ReadArchive()
    {
        std::vector<MountedFile> MountedFiles;

        const Game::ArchiveList<Game::RunlistId::File> Files(*Archive);
        int Idx = 0;
        for (auto& File : Files) {
            auto& MntFile = MountedFiles.emplace_back(MountedFile{
                .Path = File.Filename,
                .FileSize = File.FileSize,
                .UserContext = (void*)Idx++
            });
        }

        Disk.emplace(MountedFiles, Utils::Random());
        Disk->HandleFileCluster.Set([this](void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) { HandleFileCluster(Ctx, LCN, Buffer); });

        return true;
    }

    bool MountedArchive::InitializeDisk()
    {
        Disk->Initialize();
        return true;
    }

    bool MountedArchive::CreateLUT()
    {
        ArchiveLists.emplace(*Archive);

        SectionLUT.reserve(ArchiveLists->Files.size());
        for (auto& File : ArchiveLists->Files) {
            //printf("%s %s\n", File.Filename, Utils::ToHex<true>(File.SHA).c_str());
            auto& Sections = SectionLUT.emplace_back();
            uint32_t ClusterCount = Utils::Align<4096>(File.FileSize) / 4096;
            Sections.reserve(ClusterCount);

            auto GetDataItr = [&](uint32_t ChunkIdx) -> uint64_t {
                auto& Info = ArchiveLists->ChunkInfos[ChunkIdx];
                return Info.DataSector * Game::Header::GetSectorSize();
            };

            auto Itr = ArchiveLists->ChunkParts.begin() + File.ChunkPartDataStartIdx;
            auto EndItr = Itr + File.ChunkPartDataSize;
            uint32_t ItrDataOffset = 0;
            for (uint32_t i = 0; i < ClusterCount; ++i) {
                uint32_t BytesToSelect = std::min(4096llu, File.FileSize - i * 4096llu);
                auto& Cluster = Sections.emplace_back();
                int j = 0;

                do {
                    EGL3_CONDITIONAL_LOG(j < 4, LogLevel::Critical, "Not enough available sections for this cluster");
                    EGL3_CONDITIONAL_LOG(ItrDataOffset <= Itr->Size, LogLevel::Critical, "Invalid data offset");
                    if (ItrDataOffset == Itr->Size) {
                        ++Itr;
                        ItrDataOffset = 0;
                    }
                    auto& ChunkPart = *Itr;
                    auto& Section = Cluster.Sections[j];
                    Section.Ptr = GetDataItr(ChunkPart.ChunkIdx) + ChunkPart.Offset + ItrDataOffset;
                    Section.Size = std::min(ChunkPart.Size - ItrDataOffset, BytesToSelect);
                    BytesToSelect -= Section.Size;
                    ItrDataOffset += Section.Size;
                    ++j;
                } while (BytesToSelect);

                EGL3_CONDITIONAL_LOG(Cluster.TotalSize() == std::min(4096llu, File.FileSize - i * 4096llu), LogLevel::Critical, "Invalid cluster size");
            }
        }

        return true;
    }

    bool MountedArchive::CreateDisk()
    {
        Disk->Create();
        return true;
    }

    bool MountedArchive::MountDisk()
    {
        Disk->Mount();
        return true;
    }

    char MountedArchive::QueryDriveLetter() const
    {
        if (!DriveLetter) {
            DriveLetter = Disk->GetDriveLetter();
        }
        return DriveLetter;
    }

    const std::filesystem::path& MountedArchive::QueryPath() const
    {
        return Path;
    }

    const Storage::Game::ArchiveRefConst<Storage::Game::Header>& MountedArchive::QueryHeader() const
    {
        return Archive->GetHeader();
    }

    const Storage::Game::ArchiveRefConst<Storage::Game::ManifestData>& MountedArchive::QueryMeta() const
    {
        return Archive->GetManifestData();
    }

    MountedArchive::Stats MountedArchive::QueryStats() const
    {
        if (!Counter.IsValid()) {
            Counter = DriveCounter(QueryDriveLetter());
        }
        if (Counter.IsValid()) {
            return Counter.GetData();
        }
        return Stats{};
    }

    void MountedArchive::HandleFileCluster(void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) const
    {
        auto& File = SectionLUT[(size_t)Ctx];

        if (File.size() < LCN) [[unlikely]] {
            ZeroMemory(Buffer, 4096);
            return;
        }

        SectionPart* Itr = (SectionPart*)&File[LCN].Sections;
        while (Itr->Size) {
            std::copy(ArchiveLists->ChunkDatas.begin() + Itr->Ptr, ArchiveLists->ChunkDatas.begin() + Itr->Ptr + Itr->Size, Buffer);
            Buffer = (uint8_t*)Buffer + Itr->Size;
            ++Itr;
        }
    }
}