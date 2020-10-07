#include "Archive.h"

#include "../../utils/assert.h"
#include "../../utils/align.h"
#include "RunlistStream.h"
#include "GameId.h"
#include "RunlistId.h"

namespace EGL3::Storage::Game {
	Archive&& Archive::Create(const fs::path& Path, CreationData&& Data)
	{
		Archive Ar(Path);
		EGL3_ASSERT(Ar.Stream.valid(), "Archive isn't open");

		Ar.ModifyMetadataInternal(std::forward<CreationData&&>(Data));

		auto& Stream = Ar.Stream.acquire();
		{
			Ar.Header.Magic = Header::MAGIC;
			Ar.Header.ArchiveVersion = Header::VERSION_LATEST;

			Stream.seek(0, Stream::Beg);
			Stream << Ar.Header;
		}

		{
			Ar.FileRunlist.AllocationId = RUNLIST_ID_FILE;
			Ar.FileRunlist.TotalSize = 0;
			Ar.FileRunlist.RunCount = 0;

			Stream.seek(256, Stream::Beg);
			Stream << Ar.FileRunlist;
		}

		{
			Ar.ChunkPartRunlist.AllocationId = RUNLIST_ID_CHUNK_PART;
			Ar.ChunkPartRunlist.TotalSize = 0;
			Ar.ChunkPartRunlist.RunCount = 0;

			Stream.seek(512, Stream::Beg);
			Stream << Ar.ChunkPartRunlist;
		}

		{
			Ar.ChunkInfoRunlist.AllocationId = RUNLIST_ID_CHUNK_INFO;
			Ar.ChunkInfoRunlist.TotalSize = 0;
			Ar.ChunkInfoRunlist.RunCount = 0;

			Stream.seek(768, Stream::Beg);
			Stream << Ar.ChunkInfoRunlist;
		}

		{
			Ar.ChunkDataRunlist.AllocationId = RUNLIST_ID_CHUNK_DATA;
			Ar.ChunkDataRunlist.TotalSize = 0;
			Ar.ChunkDataRunlist.RunCount = 0;

			Stream.seek(1024, Stream::Beg);
			Stream << Ar.ChunkDataRunlist;
		}

		{
			Stream.seek(2048, Stream::Beg);
			Stream << Ar.ManifestData;
		}

		{
			Ar.RunIndex.NextAvailableSector = 16; // 8192 bytes
			Ar.RunIndex.ElementCount = 0;

			Stream.seek(4096, Stream::Beg);
			Stream << Ar.RunIndex;
		}

		return std::move(Ar);
	}

	Archive&& Archive::Open(const fs::path& Path)
	{
		Archive Ar(Path);
		EGL3_ASSERT(Ar.Stream.valid(), "Archive isn't open");

		auto& Stream = Ar.Stream.acquire();
		{
			Stream.seek(0, Stream::Beg);
			Stream >> Ar.Header;
		}

		{
			Stream.seek(256, Stream::Beg);
			Stream >> Ar.FileRunlist;
		}

		{
			Stream.seek(512, Stream::Beg);
			Stream >> Ar.ChunkPartRunlist;
		}

		{
			Stream.seek(768, Stream::Beg);
			Stream >> Ar.ChunkInfoRunlist;
		}

		{
			Stream.seek(1024, Stream::Beg);
			Stream >> Ar.ChunkDataRunlist;
		}

		{
			Stream.seek(2048, Stream::Beg);
			Stream >> Ar.ManifestData;
		}

		{
			Stream.seek(4096, Stream::Beg);
			Stream >> Ar.RunIndex;
		}


		return std::move(Ar);
	}

	void Archive::ModifyMetadata(CreationData&& Data)
	{
		ModifyMetadataInternal(std::forward<CreationData&&>(Data));

		auto& Stream = this->Stream.acquire();
		Stream.seek(0, Stream::Beg);
		Stream << Header;

		Stream.seek(2048, Stream::Beg);
		Stream << ManifestData;
	}

	Archive::~Archive()
	{
	}

	Archive::Archive(const fs::path& Path) :
		Header({ 0 }),
		FileRunlist({ 0 }),
		ChunkPartRunlist({ 0 }),
		ChunkInfoRunlist({ 0 }),
		ChunkDataRunlist({ 0 }),
		ManifestData({ 0 }),
		RunIndex({ 0 }),
		Backend(Path)
	{
		
	}

	void Archive::ModifyMetadataInternal(CreationData&& Data)
	{
		strcpy_s(Header.Game, Data.Game.c_str());
		strcpy_s(Header.VersionStringLong, Data.VersionStringLong.c_str());
		strcpy_s(Header.VersionStringHR, Data.VersionStringHR.c_str());
		Header.GameNumeric = Data.GameNumeric;
		Header.VersionNumeric = Data.VersionNumeric;

		strcpy_s(ManifestData.LaunchExeString, Data.LaunchExeString.c_str());
		strcpy_s(ManifestData.LaunchCommand, Data.LaunchCommand.c_str());
		strcpy_s(ManifestData.AppNameString, Data.AppNameString.c_str());
		ManifestData.AppID = Data.AppID;
		ManifestData.BaseUrls = std::move(Data.BaseUrls);
	}

	void Archive::Resize(Runlist& Runlist, int64_t NewSize)
	{
		if (Runlist.TotalSize >= NewSize) {
			return;
		}

		auto AllocatedSectors = std::accumulate(Runlist.Runs.begin(), Runlist.Runs.end(), 0ull, [](uint64_t A, const RunlistElement& B) {
			return A + B.SectorSize;
		});
		auto RequiredSectors = Utils::Align<512>(NewSize) / 512;
		if (RequiredSectors <= AllocatedSectors) {
			Runlist.TotalSize = NewSize;
			return;
		}

		// Number of more sectors that the run needs to be reach the target size
		auto MoreSectorsNeeded = RequiredSectors - AllocatedSectors;

		// We will need to modify the index/runlist past this point

		if (RunIndex.Elements.back().AllocationId == Runlist.AllocationId) {
			// We can simply increase the size of the last run here without worry of clashing
			RunIndex.Elements.back().SectorCount += MoreSectorsNeeded;
			Runlist.Runs.back().SectorSize += MoreSectorsNeeded;
			Runlist.TotalSize = NewSize;
			return;
		}

		// We need to add a new run to the index and to the runlist
		auto& IndexElement = RunIndex.Elements.emplace_back();
		IndexElement.SectorCount = MoreSectorsNeeded;
		IndexElement.AllocationId = Runlist.AllocationId;
		RunIndex.ElementCount++;

		auto& RunElement = Runlist.Runs.emplace_back();
		RunElement.SectorOffset = RunIndex.NextAvailableSector;
		RunElement.SectorSize = MoreSectorsNeeded;
		Runlist.RunCount++;

		Runlist.TotalSize = NewSize;
		RunIndex.NextAvailableSector += MoreSectorsNeeded;
	}
}
