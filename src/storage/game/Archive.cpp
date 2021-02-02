#include "Archive.h"

#include "../../utils/Assert.h"
#include "../../utils/Align.h"
#include "ArchiveList.h"

namespace EGL3::Storage::Game {
    Archive::Archive(const std::filesystem::path& Path, ArchiveMode Mode) noexcept :
        Valid(false),
        Header(*Backend, 0),
        ManifestData(*Backend, Header::GetOffsetManifestData()),
        RunlistFile(*Backend, Header::GetOffsetRunlistFile()),
        RunlistChunkPart(*Backend, Header::GetOffsetRunlistChunkPart()),
        RunlistChunkInfo(*Backend, Header::GetOffsetRunlistChunkInfo()),
        RunlistChunkData(*Backend, Header::GetOffsetManifestData()),
        RunIndex(*Backend, Header::GetOffsetRunIndex())
    {
        std::error_code Code;

        switch (Mode)
        {
        case ArchiveMode::Create:
        {
            if (EGL3_CONDITIONAL_LOG(std::filesystem::is_regular_file(Path, Code), LogLevel::Warning, "Archive file already exists, overwriting")) {
                if (!std::filesystem::remove(Path, Code)) {
                    printf("%s - %s\n", Code.category().name(), Code.message().c_str());
                    EGL3_LOG(LogLevel::Critical, "Remove error");
                }
            }
            else if (Code) {
                printf("%s - %s\n", Code.category().name(), Code.message().c_str());
                EGL3_LOG(LogLevel::Error, "Could not check if regular file");
                return;
            }

            Backend.emplace(Path, Utils::Mmio::OptionWrite);
            if (!EGL3_CONDITIONAL_LOG(Backend->IsValid(), LogLevel::Error, "Archive file could not be created")) {
                return;
            }

            Backend->EnsureSize(Header::GetMinimumArchiveSize());

            std::construct_at(Header.Get());

            std::construct_at(ManifestData.Get());

            std::construct_at(RunlistFile.Get(), RunlistId::File);
            std::construct_at(RunlistChunkPart.Get(), RunlistId::ChunkPart);
            std::construct_at(RunlistChunkInfo.Get(), RunlistId::ChunkInfo);
            std::construct_at(RunlistChunkData.Get(), RunlistId::ChunkData);

            std::construct_at(RunIndex.Get());

            Valid = true;
            break;
        }
        case ArchiveMode::Load:
        case ArchiveMode::Read:
        {
            if (!EGL3_CONDITIONAL_LOG(std::filesystem::is_regular_file(Path, Code), LogLevel::Error, "Archive file does not exist")) {
                return;
            }
            else if (Code) {
                printf("%s - %s\n", Code.category().name(), Code.message().c_str());
                EGL3_LOG(LogLevel::Error, "Could not check if regular file");
                return;
            }

            if (Mode == ArchiveMode::Load) {
                Backend.emplace(Path, Utils::Mmio::OptionWrite);
            }
            else {
                Backend.emplace(Path, Utils::Mmio::OptionRead);
            }
            if (!EGL3_CONDITIONAL_LOG(Backend->IsValid(), LogLevel::Error, "Archive file could not be opened")) {
                return;
            }

            if (!EGL3_CONDITIONAL_LOG(Backend->IsValidPosition(Header::GetMinimumArchiveSize()), LogLevel::Error, "Archive file is too small")) {
                return;
            }

            if (!EGL3_CONDITIONAL_LOG(Header->HasValidMagic(), LogLevel::Error, "Archive has invalid magic")) {
                return;
            }

            if (!EGL3_CONDITIONAL_LOG(Header->HasValidHeaderSize(), LogLevel::Error, "Archive header has invalid size")) {
                return;
            }

            if (!EGL3_CONDITIONAL_LOG(Header->GetVersion() == ArchiveVersion::Latest, LogLevel::Error, "Archive has invalid version")) {
                return;
            }

            Valid = true;
            break;
        }
        }
    }

    bool Archive::IsValid() const
    {
        return Valid;
    }

    template<uint32_t MaxRunCount>
    size_t Archive::ReadRunlist(const Runlist<MaxRunCount>& Runlist, size_t Position, char* Dst, size_t DstSize) const
    {
        if (Position + DstSize > Runlist.GetSize()) {
            DstSize = Runlist.GetSize() - Position;
        }
        uint32_t RunStartIndex, RunByteOffset;
        if (Runlist.GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
            uint32_t BytesRead = 0;
            for (auto CurrentRunItr = Runlist.GetRuns().begin() + RunStartIndex; CurrentRunItr != Runlist.GetRuns().end(); ++CurrentRunItr) {
                size_t Offset = CurrentRunItr->SectorOffset * Header::GetSectorSize() + RunByteOffset;
                if ((DstSize - BytesRead) > CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset) { // copy the entire buffer over
                    memcpy(Dst + BytesRead, Get() + Offset, CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset);
                    BytesRead += CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset;
                }
                else { // copy what it needs to fill up the rest
                    memcpy(Dst + BytesRead, Get() + Offset, DstSize - BytesRead);
                    BytesRead += DstSize - BytesRead;
                    break;
                }
                RunByteOffset = 0;
            }
            return BytesRead;
        }

        return 0;
    }

    template<uint32_t MaxRunCount>
    void Archive::Resize(ArchiveRef<Runlist<MaxRunCount>>& Runlist, uint64_t NewSize)
    {
        // Placing this after can cause a memory access exception due to the memory being moved
        Runlist->SetSize(NewSize);

        if (NewSize > Runlist->GetAllocatedSize()) {
            Reserve(Runlist, NewSize);
        }
    }

    template<uint32_t MaxRunCount>
    void Archive::Reserve(ArchiveRef<Runlist<MaxRunCount>>& Runlist, uint64_t NewAllocatedSize)
    {
        if (Runlist->GetAllocatedSize() >= NewAllocatedSize) {
            Runlist->SetAllocatedSize(NewAllocatedSize);
            return;
        }

        auto AllocatedSectors = Runlist->GetAllocatedSectors();
        auto RequiredSectors = Utils::Align<Header::GetSectorSize()>(NewAllocatedSize) / Header::GetSectorSize();
        if (AllocatedSectors >= RequiredSectors) {
            Runlist->SetAllocatedSize(NewAllocatedSize);
            return;
        }

        // Number of more sectors that the run needs to have to reach the target size
        auto MoreSectorsNeeded = RequiredSectors - AllocatedSectors;

        // We will need to modify the index/runlist past this point
        if (RunIndex->GetRunCount()) {
            if (RunIndex->GetRuns()[RunIndex->GetRunCount() - 1].Id == Runlist->GetId()) {
                // We can simply increase the size of the last run here without worry of clashing
                Runlist->ExtendLastRun(MoreSectorsNeeded);
                RunIndex->ExtendLastRun(MoreSectorsNeeded);
                Runlist->SetAllocatedSize(NewAllocatedSize);
                Backend->EnsureSize(RunIndex->GetNextAvailableSector() * Header::GetSectorSize());
                return;
            }
        }

        // We need to add a new run to the index and to the runlist
        EGL3_CONDITIONAL_LOG(Runlist->EmplaceRun(RunIndex->GetNextAvailableSector(), MoreSectorsNeeded), LogLevel::Critical, "Runlist ran out of available runs");
        EGL3_CONDITIONAL_LOG(RunIndex->EmplaceRun(MoreSectorsNeeded, Runlist->GetId()), LogLevel::Critical, "Run index ran out of available runs");
        Runlist->SetAllocatedSize(NewAllocatedSize);
        Backend->EnsureSize(RunIndex->GetNextAvailableSector() * Header::GetSectorSize());
    }

    /*
    const std::vector<EGL3File> ArchiveReadonly::GetFiles() const
    {
        std::vector<EGL3File> Ret;

        RunlistStreamReadonly FileStream(GetRunlistFile(), *this);
        while (FileStream.size() > FileStream.tell()) {
            File CurFile;
            FileStream.read((char*)&CurFile, sizeof(File));

            int64_t Parent = -1;
            std::filesystem::path Path(CurFile.Filename);
            for (auto& Section : Path) {
                auto Itr = std::find_if(Ret.begin(), Ret.end(), [&](const EGL3File& File) {
                    return File.is_directory && File.parent_index == Parent && std::is_eq(Utils::CompareStringsInsensitive(std::string(File.name), Section.string()));
                });
                if (Itr != Ret.end()) {
                    Parent = std::distance(Ret.begin(), Itr);
                }
                else {
                    Ret.emplace_back(EGL3File{
                        .name = Section.string().c_str(), // TODO: MUST FIX!
                        .size = 0,
                        .is_directory = 1,
                        .parent_index = Parent,
                        .reserved = nullptr,
                        .o_runlist = nullptr
                    });
                    Parent = Ret.size() - 1;
                }
            }
            auto& File = Ret[Parent];
            File.is_directory = 0;
            File.size = CurFile.FileSize;
        }
    }

    void ArchiveReadonly::HandleFile(const EGL3File& File, uint64_t LCN, uint8_t Buffer[4096]) const
    {
        memset(Buffer, 'A', 1024);
        memset(Buffer + 1024, 'B', 1024);
        memset(Buffer + 2048, 'C', 1024);
        memset(Buffer + 3072, 'D', 1024);
        *(uint64_t*)Buffer = LCN;
    }
    */
}
