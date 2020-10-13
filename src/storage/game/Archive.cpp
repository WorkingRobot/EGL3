#include "Archive.h"

#include "../../utils/assert.h"
#include "../../utils/align.h"
#include "RunlistStream.h"
#include "GameId.h"
#include "RunlistId.h"

#include <numeric>

namespace EGL3::Storage::Game {
	Archive::Archive(const fs::path& Path) : Backend(Path)
	{
		Header				= (decltype(Header))			Backend.Get() + 0;
		FileRunlist			= (decltype(FileRunlist))		Backend.Get() + Header->FileRunlistOffset;
		ChunkPartRunlist	= (decltype(ChunkPartRunlist))	Backend.Get() + Header->ChunkPartRunlistOffset;
		ChunkInfoRunlist	= (decltype(ChunkInfoRunlist))	Backend.Get() + Header->ChunkInfoRunlistOffset;
		ChunkDataRunlist	= (decltype(ChunkDataRunlist))	Backend.Get() + Header->ChunkDataRunlistOffset;
		RunIndex			= (decltype(RunIndex))			Backend.Get() + Header->RunIndexOffset;
		ManifestData		= (decltype(ManifestData))		Backend.Get() + Header->ManifestDataOffset;
	}

	Archive::~Archive()
	{
	}

	// TODO: check if Runlist and RunIndex's sizes can hold an extra element (and resize accordingly somehow or something)
	// TODO: ensure/change size of archive when modifying the index so we don't write past eof
	void Archive::Resize(Runlist* Runlist, int64_t NewSize)
	{
		if (Runlist->TotalSize >= NewSize) {
			return;
		}

		auto AllocatedSectors = std::accumulate(Runlist->Runs, Runlist->Runs + Runlist->RunCount, 0ull, [](uint64_t A, const RunlistElement& B) {
			return A + B.SectorSize;
		});
		auto RequiredSectors = Utils::Align<512>(NewSize) / 512;
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
			Runlist->Runs[Runlist->RunCount-1].SectorSize += MoreSectorsNeeded;
			Runlist->TotalSize = NewSize;
			return;
		}

		// We need to add a new run to the index and to the runlist
		RunIndex->Elements[RunIndex->ElementCount].SectorCount = MoreSectorsNeeded;
		RunIndex->Elements[RunIndex->ElementCount].AllocationId = Runlist->AllocationId;
		RunIndex->ElementCount++;

		Runlist->Runs[Runlist->RunCount].SectorOffset = RunIndex->NextAvailableSector;
		Runlist->Runs[Runlist->RunCount].SectorSize = MoreSectorsNeeded;
		Runlist->RunCount++;

		Runlist->TotalSize = NewSize;
		RunIndex->NextAvailableSector += MoreSectorsNeeded;
	}
}
