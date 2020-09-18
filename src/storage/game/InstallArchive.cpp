#include "install.h"

#include "../utils/assert.h"
#include "../utils/align.h"
#include "CRunlistStream.h"

namespace EGL3::Storage::Game {
	InstallArchive::InstallArchive(fs::path Path) :
		Header({ 0 }),
		FileRunlist({ 0 }),
		ChunkPartRunlist({ 0 }),
		ChunkInfoRunlist({ 0 }),
		ChunkDataRunlist({ 0 }),
		ManifestData({ 0 }),
		RunIndex({ 0 })
	{
		if (fs::is_regular_file(Path)) {
			Stream.open(Path, "rb+");
			if (!Stream.valid()) {
				// ERROR
				// File could not be opened
			}
			InitializeParse();
		}
		else {
			Stream.open(Path, "wb+");
			if (!Stream.valid()) {
				// ERROR
				// File could not be created
			}
			InitializeCreate();
		}
	}

	InstallArchive::~InstallArchive()
	{
	}

	void InstallArchive::InitializeParse()
	{
		EGL3_ASSERT(Stream.valid(), "Archive isn't open");
		Stream >> Header;

		Stream.seek(256, CStream::Beg);
		Stream >> FileRunlist;

		Stream.seek(512, CStream::Beg);
		Stream >> ChunkPartRunlist;

		Stream.seek(768, CStream::Beg);
		Stream >> ChunkInfoRunlist;

		Stream.seek(1024, CStream::Beg);
		Stream >> ChunkDataRunlist;

		Stream.seek(2048, CStream::Beg);
		Stream >> ManifestData;

		Stream.seek(4096, CStream::Beg);
		Stream >> RunIndex;
	}

	void InstallArchive::InitializeCreate()
	{
		{
			Header.Magic = InstallHeader::MAGIC;
			Header.ArchiveVersion = InstallHeader::VERSION_LATEST;
			strcpy(Header.Game, "Fortnite");
			strcpy(Header.VersionStringLong, "++Fortnite+Release-12.30-CL-12624643-Windows");
			strcpy(Header.VersionStringHR, "12.30");
			Header.GameNumeric = GAME_ID_FORTNITE;
			Header.VersionNumeric = 12624643;

			Stream.seek(0, CStream::Beg);
			Stream << Header;
		}

		{
			FileRunlist.AllocationId = RUNLIST_ID_FILE;
			FileRunlist.TotalSize = 0;
			FileRunlist.RunCount = 0;

			Stream.seek(256, CStream::Beg);
			Stream << FileRunlist;
		}

		{
			ChunkPartRunlist.AllocationId = RUNLIST_ID_CHUNK_PART;
			ChunkPartRunlist.TotalSize = 0;
			ChunkPartRunlist.RunCount = 0;

			Stream.seek(512, CStream::Beg);
			Stream << ChunkPartRunlist;
		}

		{
			ChunkInfoRunlist.AllocationId = RUNLIST_ID_CHUNK_INFO;
			ChunkInfoRunlist.TotalSize = 0;
			ChunkInfoRunlist.RunCount = 0;

			Stream.seek(768, CStream::Beg);
			Stream << ChunkInfoRunlist;
		}

		{
			ChunkDataRunlist.AllocationId = RUNLIST_ID_CHUNK_DATA;
			ChunkDataRunlist.TotalSize = 0;
			ChunkDataRunlist.RunCount = 0;

			Stream.seek(1024, CStream::Beg);
			Stream << ChunkDataRunlist;
		}

		{
			strcpy(ManifestData.LaunchExeString, "FortniteGame/Binaries/Win64/FortniteLauncher.exe");
			strcpy(ManifestData.LaunchCommand, " -obfuscationid=NaQlKPDGk9dl7U3yYEqV48dpcim8OA");
			strcpy(ManifestData.AppNameString, "FortniteReleaseBuilds");
			ManifestData.AppID = 1;
			ManifestData.BaseUrlCount = 3;
			ManifestData.BaseUrls.emplace_back("http://example.com/CloudDir");
			ManifestData.BaseUrls.emplace_back("https://cdn2.example.com/areallyreallyreallyverylongstringhereforsomereason/CloudDir");
			ManifestData.BaseUrls.emplace_back("https://cdn3.example.net/fortnite/CloudDir");

			Stream.seek(2048, CStream::Beg);
			Stream << ManifestData;
		}

		{
			RunIndex.NextAvailableSector = 16; // 8192 bytes
			RunIndex.ElementCount = 0;

			Stream.seek(4096, CStream::Beg);
			Stream << RunIndex;
		}
	}

	void InstallArchive::Resize(InstallRunlist& Runlist, int64_t NewSize)
	{
		if (Runlist.TotalSize >= NewSize) {
			return;
		}

		auto AllocatedSectors = std::accumulate(Runlist.Runs.begin(), Runlist.Runs.end(), 0ull, [](uint64_t A, const InstallRunlistElement& B) {
			return A + B.SectorSize;
			});
		auto RequiredSectors = Align<512>(NewSize) / 512;
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
