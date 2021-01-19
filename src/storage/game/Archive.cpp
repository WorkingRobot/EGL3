#include "Archive.h"

#include "../../utils/Assert.h"
#include "../../utils/Align.h"
#include "RunlistStream.h"
#include "GameId.h"
#include "RunlistId.h"

#include <numeric>

namespace EGL3::Storage::Game {
    Archive::Archive(const fs::path& Path, Detail::ECreate) noexcept :
        Valid(false)
    {
        if (EGL3_CONDITIONAL_LOG(fs::is_regular_file(Path), LogLevel::Warning, "Archive file already exists, overwriting")) {
            fs::remove(Path);
        }

        Backend.emplace(Path);
        if (!EGL3_CONDITIONAL_LOG(Backend->IsValid(), LogLevel::Error, "Archive file could not be created")) {
            return;
        }

        Backend->EnsureSize(Header::FileDataOffset);

        Header              = (decltype(Header))            Backend->Get() + 0;
        ManifestData        = (decltype(ManifestData))      Backend->Get() + Header::ManifestDataOffset;
        FileRunlist         = (decltype(FileRunlist))       Backend->Get() + Header::FileRunlistOffset;
        ChunkPartRunlist    = (decltype(ChunkPartRunlist))  Backend->Get() + Header::ChunkPartRunlistOffset;
        ChunkInfoRunlist    = (decltype(ChunkInfoRunlist))  Backend->Get() + Header::ChunkInfoRunlistOffset;
        ChunkDataRunlist    = (decltype(ChunkDataRunlist))  Backend->Get() + Header::ChunkDataRunlistOffset;
        RunIndex            = (decltype(RunIndex))          Backend->Get() + Header::RunIndexOffset;

        *Header = Game::Header{
            .Magic = Header::ExpectedMagic,
            .ArchiveVersion = ArchiveVersion::Latest,
            .HeaderSize = sizeof(Game::Header)
        };

        *FileRunlist = Runlist{ .AllocationId = RunlistId::File, .Runs{ {} } };
        *ChunkPartRunlist = Runlist{ .AllocationId = RunlistId::ChunkPart };
        *ChunkInfoRunlist = Runlist{ .AllocationId = RunlistId::ChunkInfo };
        *ChunkDataRunlist = Runlist{ .AllocationId = RunlistId::ChunkData };

        *RunIndex = Game::RunIndex{ .NextAvailableSector = Header::FileDataSectorOffset, .Elements{ {} } };

        Valid = true;
    }

    Archive::Archive(const fs::path& Path, Detail::ELoad) noexcept :
        Valid(false)
    {
        if (!EGL3_CONDITIONAL_LOG(fs::is_regular_file(Path), LogLevel::Error, "Archive file does not exist")) {
            return;
        }

        Backend.emplace(Path);
        if (!EGL3_CONDITIONAL_LOG(Backend->IsValid(), LogLevel::Error, "Archive file could not be opened")) {
            return;
        }

        if (!EGL3_CONDITIONAL_LOG(Backend->IsValidOffset(Header::FileDataOffset), LogLevel::Error, "Archive file is too small")) {
            return;
        }

        Header              = (decltype(Header))            Backend->Get() + 0;

        if (!EGL3_CONDITIONAL_LOG(Header->Magic == Header::ExpectedMagic, LogLevel::Error, "Archive has invalid magic")) {
            return;
        }

        if (!EGL3_CONDITIONAL_LOG(Header->ArchiveVersion == ArchiveVersion::Latest, LogLevel::Error, "Archive has invalid version")) {
            return;
        }

        if (!EGL3_CONDITIONAL_LOG(Header->HeaderSize == sizeof(Game::Header), LogLevel::Error, "Archive header has invalid size")) {
            return;
        }
        
        ManifestData        = (decltype(ManifestData))      Backend->Get() + Header::ManifestDataOffset;
        FileRunlist         = (decltype(FileRunlist))       Backend->Get() + Header::FileRunlistOffset;
        ChunkPartRunlist    = (decltype(ChunkPartRunlist))  Backend->Get() + Header::ChunkPartRunlistOffset;
        ChunkInfoRunlist    = (decltype(ChunkInfoRunlist))  Backend->Get() + Header::ChunkInfoRunlistOffset;
        ChunkDataRunlist    = (decltype(ChunkDataRunlist))  Backend->Get() + Header::ChunkDataRunlistOffset;
        RunIndex            = (decltype(RunIndex))          Backend->Get() + Header::RunIndexOffset;

        Valid = true;
    }

    Archive::~Archive()
    {

    }

    bool Archive::IsValid() const
    {
        return Valid;
    }

    std::string Archive::GetGame() const
    {
        return std::string(Header->Game, strnlen_s(Header->Game, sizeof(Header->Game)));
    }

    std::string Archive::GetVersionLong() const
    {
        return std::string(Header->VersionStringLong, strnlen_s(Header->VersionStringLong, sizeof(Header->VersionStringLong)));
    }

    std::string Archive::GetVersionHR() const
    {
        return std::string(Header->VersionStringHR, strnlen_s(Header->VersionStringHR, sizeof(Header->VersionStringHR)));
    }

    uint64_t Archive::GetVersionNum() const
    {
        return Header->VersionNumeric;
    }

    GameId Archive::GetGameId() const
    {
        return Header->GameNumeric;
    }

    // TODO: check if Runlist and RunIndex's sizes can hold an extra element (and resize accordingly somehow or something)
    // TODO: ensure/change size of archive when modifying the index so we don't write past eof
    void Archive::Resize(Runlist* Runlist, int64_t NewSize)
    {
        if (Runlist->TotalSize >= NewSize) {
            return;
        }

        auto AllocatedSectors = std::accumulate(Runlist->Runs, Runlist->Runs + Runlist->RunCount, 0ull, [](uint64_t A, const RunlistElement& B) {
            return A + B.SectorCount;
        });
        auto RequiredSectors = Utils::Align<Header::SectorSize>(NewSize) / Header::SectorSize;
        if (RequiredSectors <= AllocatedSectors) {
            Runlist->TotalSize = NewSize;
            return;
        }

        // Number of more sectors that the run needs to be reach the target size
        auto MoreSectorsNeeded = RequiredSectors - AllocatedSectors;

        // We will need to modify the index/runlist past this point

        if (RunIndex->Elements[RunIndex->ElementCount-1].AllocationId == Runlist->AllocationId) {
            // We can simply increase the size of the last run here without worry of clashing
            RunIndex->Elements[RunIndex->ElementCount - 1].SectorCount += MoreSectorsNeeded;
            Runlist->Runs[Runlist->RunCount-1].SectorCount += MoreSectorsNeeded;
            Runlist->TotalSize = NewSize;
            return;
        }

        // We need to add a new run to the index and to the runlist
        RunIndex->Elements[RunIndex->ElementCount].SectorCount = MoreSectorsNeeded;
        RunIndex->Elements[RunIndex->ElementCount].AllocationId = Runlist->AllocationId;
        RunIndex->ElementCount++;

        Runlist->Runs[Runlist->RunCount].SectorOffset = RunIndex->NextAvailableSector;
        Runlist->Runs[Runlist->RunCount].SectorCount = MoreSectorsNeeded;
        Runlist->RunCount++;

        Runlist->TotalSize = NewSize;
        RunIndex->NextAvailableSector += MoreSectorsNeeded;
    }
}
