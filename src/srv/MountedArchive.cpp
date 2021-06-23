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
        Contexts.reserve(ArchiveLists.Files.size());

        for (auto& File : ArchiveLists.Files) {
            std::vector<std::pair<uint32_t, uint32_t>> LUT;
            std::vector<std::pair<Storage::Game::ArchiveListIteratorReadonly<Storage::Game::RunlistId::ChunkData>, uint32_t>> Parts;

            uint32_t ClusterCount = Utils::Align<4096>(File.FileSize) / 4096;
            LUT.reserve(ClusterCount);
            Parts.reserve(File.ChunkPartDataSize);

            auto GetDataItr = [&](uint32_t ChunkIdx) -> uint64_t {
                auto& Info = ArchiveLists.ChunkInfos[ChunkIdx];
                return Info.DataSector * Game::Header::GetSectorSize();
            };

            auto PartBegin = ArchiveLists.ChunkParts.begin() + File.ChunkPartDataStartIdx;
            auto PartEnd = PartBegin + File.ChunkPartDataSize;
            uint32_t PartIdx = 0;
            uint32_t Offset = 0;
            for (auto PartItr = PartBegin; PartItr != PartEnd; ++PartItr) {
                auto& Part = *PartItr;
                Parts.emplace_back(ArchiveLists.ChunkDatas.begin() + GetDataItr(Part.ChunkIdx) + Part.Offset, Part.Size);

                while (LUT.size() < ClusterCount) {
                    LUT.emplace_back(PartIdx, Offset);
                    Offset += 4096;
                    if (Offset >= Part.Size) {
                        Offset -= Part.Size;
                        break;
                    }
                }

                ++PartIdx;
            }

            Contexts.emplace_back(std::move(LUT), std::move(Parts));
        }
    }

    MountedArchive::SectionContext::SectionContext(std::vector<std::pair<uint32_t, uint32_t>>&& LUT, std::vector<std::pair<Storage::Game::ArchiveListIteratorReadonly<Storage::Game::RunlistId::ChunkData>, uint32_t>>&& Parts) :
        LUT(std::move(LUT)),
        Parts(std::move(Parts))
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

#if 1
    void MountedArchive::HandleCluster(void* CtxPtr, uint32_t LCN, uint32_t Count, uint8_t* Buffer) noexcept {
        Count *= 4096;

        auto& Ctx = *(MountedArchive::SectionContext*)CtxPtr;

        auto PartItr = Ctx.Parts.begin() + Ctx.LUT[LCN].first;
        uint32_t PartOffset = Ctx.LUT[LCN].second;

        Storage::Game::PrefetchTableSrcEntry SrcTable[16];
        void* DstTable[16];

        Storage::Game::PrefetchTableSrcEntry* SrcTablePtr = SrcTable;
        void** DstTablePtr = DstTable;

        while (Count && PartItr != Ctx.Parts.end()) {
            auto Ptr = PartItr->first + PartOffset;
            auto ReadAmt = std::min(PartItr->second - PartOffset, Count);

            auto TableCnt = Ptr.QueueFastPrefetch((char*)Buffer, ReadAmt, SrcTablePtr, DstTablePtr);
            SrcTablePtr += TableCnt;
            DstTablePtr += TableCnt;

            Buffer += ReadAmt;
            PartOffset = 0;
            ++PartItr;
            Count -= ReadAmt;
        }

        // printf("%d\n", DstTablePtr - DstTable);
        PrefetchVirtualMemory(GetCurrentProcess(), DstTablePtr - DstTable, (PWIN32_MEMORY_RANGE_ENTRY)SrcTable, 0);
        for (int i = 0; i < DstTablePtr - DstTable; ++i) {
            memcpy(DstTable[i], SrcTable[i].VirtualAddress, SrcTable[i].NumberOfBytes);
        }
    }
#else
    void MountedArchive::HandleCluster(void* CtxPtr, uint32_t LCN, uint32_t Count, uint8_t* Buffer) noexcept {
        Count *= 4096;

        auto& Ctx = *(MountedArchive::SectionContext*)CtxPtr;

        auto PartItr = Ctx.Parts.begin() + Ctx.LUT[LCN].first;
        uint32_t PartOffset = Ctx.LUT[LCN].second;

        while (ByteCount && PartItr != Ctx.Parts.end()) {
            auto Ptr = PartItr->first + PartOffset;
            auto ReadAmt = std::min(PartItr->second - PartOffset, Count);

            Ptr.FastRead((char*)Buffer, ReadAmt);

            Buffer += ReadAmt;
            PartOffset = 0;
            ++PartItr;
            Count -= ReadAmt;
        }
    }
#endif
}