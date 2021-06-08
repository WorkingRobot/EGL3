#include "Archive.h"

#include "../../utils/formatters/Path.h"
#include "../../utils/Log.h"
#include "../../utils/Align.h"
#include "ArchiveList.h"

namespace EGL3::Storage::Game {
    Archive::Archive(const std::filesystem::path& Path, ArchiveMode Mode) noexcept :
        Valid(false)
    {
        std::error_code Code;

        switch (Mode)
        {
        case ArchiveMode::Create:
        {
            auto IsRegularFile = std::filesystem::exists(Path, Code) && std::filesystem::is_regular_file(Path, Code);
            if (Code) {
                EGL3_LOGF(LogLevel::Error, "Could not check if regular file ({} - {})", Code.category().name(), Code.message());
                return;
            }
            else if (!EGL3_ENSURE(!IsRegularFile, LogLevel::Warning, "Archive file already exists, overwriting")) {
                std::filesystem::remove(Path, Code);
                if (Code) {
                    EGL3_ABORTF("Could not delete archive file to overwrite with a new one ({} - {})", Code.category().name(), Code.message());
                }
            }

            Backend.emplace(Path, Utils::Mmio::OptionWrite);
            if (!EGL3_ENSURE(Backend->IsValid(), LogLevel::Error, "Archive file could not be created")) {
                return;
            }

            Construct();

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
            EGL3_LOGF(LogLevel::Info, "Opening {}", Path);
            if (!EGL3_ENSURE(std::filesystem::is_regular_file(Path, Code), LogLevel::Error, "Archive file does not exist")) {
                return;
            }
            else if (Code) {
                EGL3_LOGF(LogLevel::Error, "Could not check if regular file ({} - {})", Code.category().name(), Code.message());
                return;
            }

            if (Mode == ArchiveMode::Load) {
                Backend.emplace(Path, Utils::Mmio::OptionWrite);
            }
            else {
                Backend.emplace(Path, Utils::Mmio::OptionRead);
            }
            if (!EGL3_ENSURE(Backend->IsValid(), LogLevel::Error, "Archive file could not be opened")) {
                return;
            }

            Construct();

            if (!EGL3_ENSURE(Backend->IsValidPosition(Header::GetMinimumArchiveSize()), LogLevel::Error, "Archive file is too small")) {
                return;
            }

            if (!EGL3_ENSURE(Header->HasValidMagic(), LogLevel::Error, "Archive has invalid magic")) {
                return;
            }

            if (!EGL3_ENSURE(Header->HasValidHeaderSize(), LogLevel::Error, "Archive header has invalid size")) {
                return;
            }

            if (!EGL3_ENSURE(Header->GetVersion() == ArchiveVersion::Latest, LogLevel::Error, "Archive has invalid version")) {
                return;
            }

            Valid = true;
            break;
        }
        }
    }

    Archive::Archive(Archive&& Other) noexcept :
        Valid(Other.Valid),
        Backend(std::move(Other.Backend))
    {
        if (Backend.has_value() && Backend->IsValid() && IsValid()) {
            Construct();
        }
    }

    void Archive::Construct()
    {
        Header = ArchiveRef<Game::Header>(*Backend, 0);
        ManifestData = ArchiveRef<Game::ManifestData>(*Backend, Header::GetOffsetManifestData());
        RunlistFile = ArchiveRef<RunlistTraits<RunlistId::File>::Runlist>(*Backend, Header::GetOffsetRunlistFile());
        RunlistChunkPart = ArchiveRef<RunlistTraits<RunlistId::ChunkPart>::Runlist>(*Backend, Header::GetOffsetRunlistChunkPart());
        RunlistChunkInfo = ArchiveRef<RunlistTraits<RunlistId::ChunkInfo>::Runlist>(*Backend, Header::GetOffsetRunlistChunkInfo());
        RunlistChunkData = ArchiveRef<RunlistTraits<RunlistId::ChunkData>::Runlist>(*Backend, Header::GetOffsetRunlistChunkData());
        RunIndex = ArchiveRef<RunIndexTraits::RunIndex>(*Backend, Header::GetOffsetRunIndex());
    }

    template<uint32_t MaxRunCount>
    size_t Archive::ReadRunlist(const Runlist<MaxRunCount>& Runlist, size_t Position, char* Dst, size_t DstSize) const
    {
        if (Position + DstSize > Runlist.GetSize()) {
            DstSize = Runlist.GetSize() - Position;
        }
        uint32_t RunStartIndex;
        uint64_t RunByteOffset;
        if (Runlist.GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
            size_t BytesRead = 0;
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
        if (NewSize > Runlist->GetAllocatedSize()) {
            Reserve(Runlist, NewSize);
        }

        Runlist->SetSize(NewSize);
    }

    template<uint32_t MaxRunCount>
    void Archive::Reserve(ArchiveRef<Runlist<MaxRunCount>>& Runlist, uint64_t NewAllocatedSize)
    {
        std::lock_guard Guard(ResizeMutex);

        if (Runlist->GetAllocatedSize() >= NewAllocatedSize) {
            Runlist->SetAllocatedSize(NewAllocatedSize);
            return;
        }

        uint64_t AllocatedSectors = Runlist->GetAllocatedSectors();
        uint64_t RequiredSectors = Utils::Align<Header::GetSectorSize()>(NewAllocatedSize) / Header::GetSectorSize();
        if (AllocatedSectors >= RequiredSectors) {
            Runlist->SetAllocatedSize(NewAllocatedSize);
            return;
        }

        // Number of more sectors that the run needs to have to reach the target size
        uint64_t MoreSectorsNeeded = RequiredSectors - AllocatedSectors;

        // We will need to modify the index/runlist past this point
        if (RunIndex->GetRunCount() > 0) {
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
        EGL3_VERIFY(Runlist->EmplaceRun(RunIndex->GetNextAvailableSector(), MoreSectorsNeeded), "Runlist ran out of available runs");
        EGL3_VERIFY(RunIndex->EmplaceRun(MoreSectorsNeeded, Runlist->GetId()), "Run index ran out of available runs");
        Runlist->SetAllocatedSize(NewAllocatedSize);
        Backend->EnsureSize(RunIndex->GetNextAvailableSector() * Header::GetSectorSize());
    }

    template<uint32_t MaxRunCount>
    void Archive::FlushRunlist(const Runlist<MaxRunCount>& Runlist, size_t Position, size_t Size)
    {
        uint32_t RunStartIndex;
        uint64_t RunByteOffset;
        if (Runlist.GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
            size_t BytesFlushed = 0;
            for (auto CurrentRunItr = Runlist.GetRuns().begin() + RunStartIndex; CurrentRunItr != Runlist.GetRuns().end(); ++CurrentRunItr) {
                size_t Offset = CurrentRunItr->SectorOffset * Header::GetSectorSize() + RunByteOffset;
                if ((Size - BytesFlushed) > CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset) { // flush the entire buffer
                    Backend->Flush(Offset, CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset);
                    BytesFlushed += CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset;
                }
                else { // flush the rest
                    Backend->Flush(Offset, Size - BytesFlushed);
                    BytesFlushed += Size - BytesFlushed;
                    break;
                }
                RunByteOffset = 0;
            }
        }
    }
}
