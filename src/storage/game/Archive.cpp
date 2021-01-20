#include "Archive.h"

#include "../../utils/Assert.h"
#include "../../utils/Align.h"

namespace EGL3::Storage::Game {
    Archive::Archive(const std::filesystem::path& Path, Detail::ECreate) noexcept :
        ArchiveReadonly()
    {
        if (EGL3_CONDITIONAL_LOG(std::filesystem::is_regular_file(Path), LogLevel::Warning, "Archive file already exists, overwriting")) {
            std::filesystem::remove(Path);
        }

        Backend = std::make_unique<Utils::Mmio::MmioFile>(Path);
        if (!EGL3_CONDITIONAL_LOG(GetBackend().IsValid(), LogLevel::Error, "Archive file could not be created")) {
            return;
        }

        GetBackend().EnsureSize(Header::GetMinimumArchiveSize());
        
        GetHeader() = Game::Header();

        GetRunlistFile() = RunlistFile(RunlistId::File);
        GetRunlistChunkPart() = RunlistChunk(RunlistId::ChunkPart);
        GetRunlistChunkInfo() = RunlistChunk(RunlistId::ChunkInfo);
        GetRunlistChunkData() = RunlistChunk(RunlistId::ChunkData);

        GetRunIndex() = RunIndexBase();

        Valid = true;
    }

    Archive::Archive(const std::filesystem::path& Path, Detail::ELoad) noexcept :
        ArchiveReadonly(Path)
    {

    }

    Utils::Mmio::MmioFile& Archive::GetBackend()
    {
        return *(Utils::Mmio::MmioFile*)Backend.get();
    }

    Header& Archive::GetHeader()
    {
        return *(Header*)(GetBackend().Get() + 0);
    }

    ManifestData& Archive::GetManifestData()
    {
        return *(ManifestData*)(GetBackend().Get() + GetHeader().GetOffsetManifestData());
    }

    Archive::RunlistFile& Archive::GetRunlistFile()
    {
        return *(RunlistFile*)(GetBackend().Get() + GetHeader().GetOffsetRunlistFile());
    }

    Archive::RunlistChunk& Archive::GetRunlistChunkPart()
    {
        return *(RunlistChunk*)(GetBackend().Get() + GetHeader().GetOffsetRunlistChunkPart());
    }

    Archive::RunlistChunk& Archive::GetRunlistChunkInfo()
    {
        return *(RunlistChunk*)(GetBackend().Get() + GetHeader().GetOffsetRunlistChunkInfo());
    }

    Archive::RunlistChunk& Archive::GetRunlistChunkData()
    {
        return *(RunlistChunk*)(GetBackend().Get() + GetHeader().GetOffsetRunlistChunkData());
    }

    Archive::RunIndexBase& Archive::GetRunIndex()
    {
        return *(RunIndexBase*)(GetBackend().Get() + GetHeader().GetOffsetRunIndex());
    }

    // TODO: check if Runlist and RunIndex's sizes can hold an extra element (and resize accordingly somehow or something)
    template<uint32_t MaxRunCount>
    void Archive::Resize(Runlist<MaxRunCount>& Runlist, uint64_t NewSize)
    {
        if (Runlist.GetValidSize() >= NewSize) {
            return;
        }

        auto AllocatedSectors = Runlist.GetAllocatedSectors();
        auto RequiredSectors = Utils::Align<Header::GetSectorSize()>(NewSize) / Header::GetSectorSize();
        if (RequiredSectors <= AllocatedSectors) {
            Runlist.SetValidSize(NewSize);
            return;
        }

        // Number of more sectors that the run needs to have to reach the target size
        auto MoreSectorsNeeded = RequiredSectors - AllocatedSectors;

        // We will need to modify the index/runlist past this point
        auto& RunIndex = GetRunIndex();
        if (RunIndex.GetRunCount()) {
            if (RunIndex.GetRuns()[RunIndex.GetRunCount() - 1].AllocationId == Runlist.GetId()) {
                // We can simply increase the size of the last run here without worry of clashing
                Runlist.ExtendLastRun(MoreSectorsNeeded);
                RunIndex.ExtendLastRun(MoreSectorsNeeded);
                Runlist.SetValidSize(NewSize);
                GetBackend().EnsureSize(RunIndex.GetNextAvailableSector() * Header::GetSectorSize());
                return;
            }
        }

        // We need to add a new run to the index and to the runlist
        EGL3_CONDITIONAL_LOG(Runlist.EmplaceRun(RunIndex.GetNextAvailableSector(), MoreSectorsNeeded), LogLevel::Critical, "Runlist ran out of available runs");
        EGL3_CONDITIONAL_LOG(RunIndex.EmplaceRun(MoreSectorsNeeded, Runlist.GetId()), LogLevel::Critical, "Run index ran out of available runs");
        Runlist.SetValidSize(NewSize);
        GetBackend().EnsureSize(RunIndex.GetNextAvailableSector() * Header::GetSectorSize());
    }
}
