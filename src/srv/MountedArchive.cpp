#include "MountedArchive.h"

#include "../utils/Align.h"
#include "../utils/Random.h"

namespace EGL3::Service {
    using namespace Storage;

    MountedArchive::MountedArchive(const std::filesystem::path& Path, Game::Archive&& Archive) :
        Archive(std::move(Archive)),
        ArchiveLists(this->Archive),
        LUT(ArchiveLists),
        Disk(GetMountedFiles(), Utils::Random(), HandleCluster),
        DriveLetter(0)
    {
        Disk.Initialize();

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

    MountedArchive::SectionLUT::SectionLUT(const Lists& ArchiveLists)
    {
        LUT.reserve(ArchiveLists.Files.size());

        for (auto& File : ArchiveLists.Files) {
            auto& Sections = LUT.emplace_back();
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

                Sections.emplace_back(Parts.size());
                do {
                    if (ItrDataOffset == Itr->Size) {
                        ++Itr;
                        ItrDataOffset = 0;
                    }
                    auto& ChunkPart = *Itr;
                    auto SectionPartSize = std::min(ChunkPart.Size - ItrDataOffset, BytesToSelect);
                    Parts.emplace_back(
                        GetDataItr(ChunkPart.ChunkIdx) + ChunkPart.Offset + ItrDataOffset,
                        SectionPartSize
                    );
                    BytesToSelect -= SectionPartSize;
                    ItrDataOffset += SectionPartSize;
                    EGL3_VERIFY(ItrDataOffset <= Itr->Size, "Invalid data offset");
                } while (BytesToSelect);
                Parts.emplace_back();
            }

            Contexts.emplace_back(Sections, Parts, ArchiveLists.ChunkDatas.begin());
        }
    }

    MountedArchive::SectionContext::SectionContext(const std::vector<uint32_t>& const LUT, const std::vector<SectionPart>& const Parts, const Storage::Game::ArchiveListIteratorReadonly<Storage::Game::RunlistId::ChunkData> DataBegin) :
        LUT(LUT),
        Parts(Parts),
        DataBegin(DataBegin)
    {

    }

    std::vector<MountedFile> MountedArchive::GetMountedFiles()
    {
        std::vector<MountedFile> MountedFiles;
        MountedFiles.reserve(ArchiveLists.Files.size());

        size_t Idx = 0;
        for (auto& File : ArchiveLists.Files) {
            MountedFiles.push_back({
                .Path = File.Filename,
                .FileSize = File.FileSize,
                .UserContext = (void*)&LUT.Contexts[Idx++]
            });
        }

        return MountedFiles;
    }

    void MountedArchive::HandleCluster(void* CtxPtr, uint64_t LCN, uint8_t Buffer[4096]) noexcept {
        auto& Ctx = *(MountedArchive::SectionContext*)CtxPtr;

        for (auto Itr = Ctx.Parts.begin() + Ctx.LUT[LCN]; Itr->IsValid(); ++Itr) {
            auto Ptr = Ctx.DataBegin + Itr->GetPtr();
            Ptr.FastRead((char*)Buffer, Itr->GetSize());
            Buffer = (uint8_t*)Buffer + Itr->GetSize();
        }
    }
}