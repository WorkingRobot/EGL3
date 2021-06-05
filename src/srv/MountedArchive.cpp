#include "MountedArchive.h"

#include "../utils/Align.h"
#include "../utils/Random.h"

namespace EGL3::Service {
    using namespace Storage;

    std::vector<MountedFile> GetMountFiles(const Game::ArchiveList<Game::RunlistId::File>& Files)
    {
        std::vector<MountedFile> MountedFiles;
        MountedFiles.reserve(Files.size());

        int Idx = 0;
        for (auto& File : Files) {
            MountedFiles.push_back({
                .Path = File.Filename,
                .FileSize = File.FileSize,
                .UserContext = (void*)Idx++
            });
        }

        return MountedFiles;
    }

    MountedArchive::MountedArchive(const std::filesystem::path& Path, Storage::Game::Archive&& Archive) :
        DriveLetter(0),
        Archive(std::move(Archive)),
        ArchiveLists(this->Archive),
        Disk(GetMountFiles(ArchiveLists.Files), Utils::Random())
    {
        Disk.HandleFileCluster.Set([this](void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) { HandleFileCluster(Ctx, LCN, Buffer); });

        Disk.Initialize();

        SectionLUT.reserve(ArchiveLists.Files.size());
        for (auto& File : ArchiveLists.Files) {
            auto& Sections = SectionLUT.emplace_back();
            uint32_t ClusterCount = Utils::Align<4096>(File.FileSize) / 4096;
            Sections.reserve(ClusterCount);

            auto GetDataItr = [&](uint32_t ChunkIdx) -> uint64_t {
                auto& Info = ArchiveLists.ChunkInfos[ChunkIdx];
                return Info.DataSector * Game::Header::GetSectorSize();
            };

            auto Itr = ArchiveLists.ChunkParts.begin() + File.ChunkPartDataStartIdx;
            auto EndItr = Itr + File.ChunkPartDataSize;
            uint32_t ItrDataOffset = 0;
            for (uint32_t i = 0; i < ClusterCount; ++i) {
                uint32_t BytesToSelect = std::min(4096llu, File.FileSize - i * 4096llu);

                Sections.emplace_back(SectionParts.size());
                do {
                    if (ItrDataOffset == Itr->Size) {
                        ++Itr;
                        ItrDataOffset = 0;
                    }
                    auto& ChunkPart = *Itr;
                    auto SectionPartSize = std::min(ChunkPart.Size - ItrDataOffset, BytesToSelect);
                    SectionParts.emplace_back(
                        GetDataItr(ChunkPart.ChunkIdx) + ChunkPart.Offset + ItrDataOffset,
                        SectionPartSize
                    );
                    BytesToSelect -= SectionPartSize;
                    ItrDataOffset += SectionPartSize;
                    EGL3_VERIFY(ItrDataOffset <= Itr->Size, "Invalid data offset");
                } while (BytesToSelect);
                SectionParts.emplace_back();
            }
        }

        Disk.Create();

        Disk.Mount();
    }

    MountedArchive::~MountedArchive()
    {
    }

    char MountedArchive::GetDriveLetter() const
    {
        if (!DriveLetter) {
            DriveLetter = Disk.GetDriveLetter();
        }
        return DriveLetter;
    }

    std::filesystem::path MountedArchive::GetArchivePath() const
    {
        return Archive.GetPath();
    }

    MountedArchive::Stats MountedArchive::QueryStats() const
    {
        if (!Counter.IsValid()) {
            Counter = DriveCounter(GetDriveLetter());
        }
        if (Counter.IsValid()) {
            return Counter.GetData();
        }
        return Stats{};
    }

    void MountedArchive::HandleFileCluster(void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) const noexcept
    {
        auto& File = SectionLUT[(size_t)Ctx];

        if (File.size() < LCN) [[unlikely]] {
            ZeroMemory(Buffer, 4096);
            return;
        }

        for(auto Itr = SectionParts.begin() + File[LCN]; Itr->IsValid(); ++Itr) {
            auto Ptr = ArchiveLists.ChunkDatas.begin() + Itr->GetPtr();
            Ptr.FastCopy((char*)Buffer, Itr->GetSize());
            Buffer = (uint8_t*)Buffer + Itr->GetSize();
        }
    }
}