#pragma once

#include "../utils/stream/CFileStream.h"

#include <stdint.h>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

enum InstallGameId {
	GAME_ID_UNKNOWN		= 0x0000000000000000,
	GAME_ID_FORTNITE	= 0x48574C5157555249,	// 'IRUWQLWH' (FORTNITE caesar ciphered +3)
};

enum InstallRunlistId {
	RUNLIST_ID_FILE				= 0x11111111,
	RUNLIST_ID_CHUNK_PART		= 0x22222222,
	RUNLIST_ID_CHUNK_INFO		= 0x33333333,
	RUNLIST_ID_CHUNK_DATA		= 0x44444444,
};

struct InstallHeader {
	uint32_t Magic;					// 'EGIA' aka 0x41494745
	uint32_t ArchiveVersion;		// Version of the install archive, currently 0
	char Game[40];					// Game name - example: Fortnite
	char VersionStringLong[64];		// Example: ++Fortnite+Release-13.30-CL-13884634-Windows
	char VersionStringHR[16];		// Human readable version - example: 13.30
	uint64_t GameNumeric;			// Should be some sort of enum (InstallGameId)
	uint64_t VersionNumeric;		// Game specific, but a new update will always have a higher number (e.g. Fortnite's P4 CL)

	enum Constants {
		MAGIC = 0x41494745,

		VERSION_INITIAL = 0,

		VERSION_LATEST_PLUS_ONE,
		VERSION_LATEST = VERSION_LATEST_PLUS_ONE - 1
	};

	friend CStream& operator>>(CStream& Stream, InstallHeader& Header) {
		Stream >> Header.Magic;
		Stream >> Header.ArchiveVersion;
		Stream >> Header.Game;
		Stream >> Header.VersionStringLong;
		Stream >> Header.VersionStringHR;
		Stream >> Header.GameNumeric;
		Stream >> Header.VersionNumeric;

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallHeader& Header) {
		Stream << Header.Magic;
		Stream << Header.ArchiveVersion;
		Stream << Header.Game;
		Stream << Header.VersionStringLong;
		Stream << Header.VersionStringHR;
		Stream << Header.GameNumeric;
		Stream << Header.VersionNumeric;

		return Stream;
	}
};

struct InstallManifestData {
	char LaunchExeString[256];		// Path to the executable to launch
	char LaunchCommand[256];		// Extra command arguments to add to the executable
	char AppNameString[64];			// Manifest-specific app name - example: FortniteReleaseBuilds
	uint32_t AppID;					// Manifest-specific app id - example: 1
	uint32_t BaseUrlCount;			// Base urls (or CloudDirs), used to download chunks
	// A list of zero terminated strings (no alignment)
	// Example: ABCD\0DEFG\0 is a url count of 2 with ABCD and DEFG
	std::vector<std::string> BaseUrls;

	friend CStream& operator>>(CStream& Stream, InstallManifestData& ManifestData) {
		Stream >> ManifestData.LaunchExeString;
		Stream >> ManifestData.LaunchCommand;
		Stream >> ManifestData.AppNameString;
		Stream >> ManifestData.AppID;
		Stream >> ManifestData.BaseUrlCount;
		for (uint32_t i = 0; i < ManifestData.BaseUrlCount; ++i) {
			auto& BaseUrl = ManifestData.BaseUrls.emplace_back();
			Stream >> BaseUrl;
		}

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallManifestData& ManifestData) {
		Stream << ManifestData.LaunchExeString;
		Stream << ManifestData.LaunchCommand;
		Stream << ManifestData.AppNameString;
		Stream << ManifestData.AppID;
		Stream << ManifestData.BaseUrlCount;
		for (auto& BaseUrl : ManifestData.BaseUrls) {
			Stream << BaseUrl;
		}

		return Stream;
	}
};

struct InstallRunlistElement {
	uint32_t SectorOffset;			// 1 sector = 512 bytes, this is the offset in sectors from the beginning of the install archive
	uint32_t SectorSize;			// Number of 512 byte sectors this run consists of

	friend CStream& operator>>(CStream& Stream, InstallRunlistElement& RunlistElement) {
		Stream >> RunlistElement.SectorOffset;
		Stream >> RunlistElement.SectorSize;

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallRunlistElement& RunlistElement) {
		Stream << RunlistElement.SectorOffset;
		Stream << RunlistElement.SectorSize;

		return Stream;
	}
};

struct InstallRunlist {
	int64_t TotalSize;							// Total byte size of all the runs. Not required to be, but internal data can make it aligned.
	uint32_t AllocationId;						// Basically the id of the runlist (unique), used in the RunIndex
	uint32_t RunCount;							// Number of runs in the list below
	std::vector<InstallRunlistElement> Runs;	// List of runs showing where the data can be found

	friend CStream& operator>>(CStream& Stream, InstallRunlist& Runlist) {
		Stream >> Runlist.TotalSize;
		Stream >> Runlist.AllocationId;
		Stream >> Runlist.RunCount;
		for (uint32_t i = 0; i < Runlist.RunCount; ++i) {
			auto& Run = Runlist.Runs.emplace_back();
			Stream >> Run;
		}

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallRunlist& Runlist) {
		Stream << Runlist.TotalSize;
		Stream << Runlist.AllocationId;
		Stream << Runlist.RunCount;
		for (auto& Run : Runlist.Runs) {
			Stream << Run;
		}

		return Stream;
	}

	bool GetRunIndex(uint64_t ByteOffset, uint32_t& RunIndex, uint32_t& RunByteOffset) const
	{
		for (RunIndex = 0; RunIndex < Runs.size(); ++RunIndex) {
			if (ByteOffset < Runs[RunIndex].SectorSize * 512ull) {
				RunByteOffset = ByteOffset;
				return true;
			}
			ByteOffset -= Runs[RunIndex].SectorSize * 512ull;
		}
		return false;
	}
};

struct InstallRunIndexElement {
	uint32_t SectorCount;			// The number of sectors this element has
	uint32_t AllocationId;			// The id of the runlist that this element is allocated to

	friend CStream& operator>>(CStream& Stream, InstallRunIndexElement& RunIndexElement) {
		Stream >> RunIndexElement.SectorCount;
		Stream >> RunIndexElement.AllocationId;

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallRunIndexElement& RunIndexElement) {
		Stream << RunIndexElement.SectorCount;
		Stream << RunIndexElement.AllocationId;

		return Stream;
	}
};

struct InstallRunIndex {
	uint32_t ElementCount;
	uint32_t NextAvailableSector;
	std::vector<InstallRunIndexElement> Elements;

	friend CStream& operator>>(CStream& Stream, InstallRunIndex& RunIndex) {
		Stream >> RunIndex.ElementCount;
		Stream >> RunIndex.NextAvailableSector;
		for (uint32_t i = 0; i < RunIndex.ElementCount; ++i) {
			auto& Run = RunIndex.Elements.emplace_back();
			Stream >> Run;
		}

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallRunIndex& RunIndex) {
		Stream << RunIndex.ElementCount;
		Stream << RunIndex.NextAvailableSector;
		for (auto& Run : RunIndex.Elements) {
			Stream << Run;
		}

		return Stream;
	}
};

struct InstallFile {
	char Filename[236];				// Engine/Binaries/ThirdParty/CEF3/Win64/chrome_elf.dll
	char SHA[20];					// SHA1 hash of the entire file
	int64_t ChunkPartDataOffset;	// Offset to the first chunk part (in chunk part data runlist)
	int64_t ChunkPartDataSize;		// Number of chunk part data in bytes, 8 byte aligned (it always is atm)
	int64_t FileSize;				// Size of the file (calculated from chunk part data)

	friend CStream& operator>>(CStream& Stream, InstallFile& File) {
		Stream >> File.Filename;
		Stream >> File.SHA;
		Stream >> File.ChunkPartDataOffset;
		Stream >> File.ChunkPartDataSize;
		Stream >> File.FileSize;

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallFile& File) {
		Stream << File.Filename;
		Stream << File.SHA;
		Stream << File.ChunkPartDataOffset;
		Stream << File.ChunkPartDataSize;
		Stream << File.FileSize;

		return Stream;
	}
};

struct InstallChunkPart {
	uint64_t Id;					// ID of the chunk, used to calculate the offset where it can be found in the chunk info runlist
	uint32_t Offset;				// Offset of the chunk data to start reading from
	uint32_t Size;					// Number of bytes of the chunk data to read from

	friend CStream& operator>>(CStream& Stream, InstallChunkPart& ChunkPart) {
		Stream >> ChunkPart.Id;
		Stream >> ChunkPart.Offset;
		Stream >> ChunkPart.Size;

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallChunkPart& ChunkPart) {
		Stream << ChunkPart.Id;
		Stream << ChunkPart.Offset;
		Stream << ChunkPart.Size;

		return Stream;
	}
};

struct InstallChunkInfo {
	char SHA[20];					// SHA hash of the chunk to check against
	uint32_t UncompressedSize;		// Uncompressed, total size that the chunk can provide (a value of 0xFFFFFFFF means uncompressed)
	uint32_t DataSize;				// Physical size of the chunk in the data runlist (this can mean compressed size, or uncompressed size if the chunk is uncompressed)
	uint32_t DataSector;			// Sector of where the chunk data starts. This is the 512-byte offset of the data in the ChunkDataRunlist (not a sector like in a runlist exactly!)
	char Guid[16];					// GUID of the chunk, used in EGL's manifests, unique
	uint64_t Hash;					// Hash of the chunk, used in EGL's manifests, used to get chunk url
	uint8_t DataGroup;				// Data group of the chunk, used in EGL's manifests, used to get chunk url
	// 1 byte padding
	uint16_t Flags;					// Flags of the chunk, marks multiple things, like compression method
	// 4 byte padding (8 byte alignment)

	friend CStream& operator>>(CStream& Stream, InstallChunkInfo& ChunkInfo) {
		Stream >> ChunkInfo.SHA;
		Stream >> ChunkInfo.UncompressedSize;
		Stream >> ChunkInfo.DataSize;
		Stream >> ChunkInfo.DataSector;
		Stream >> ChunkInfo.Guid;
		Stream >> ChunkInfo.Hash;
		Stream >> ChunkInfo.DataGroup;
		Stream.seek(1, CStream::Cur);
		Stream >> ChunkInfo.Flags;
		Stream.seek(4, CStream::Cur);

		return Stream;
	}

	friend CStream& operator<<(CStream& Stream, InstallChunkInfo& ChunkInfo) {
		Stream << ChunkInfo.SHA;
		Stream << ChunkInfo.UncompressedSize;
		Stream << ChunkInfo.DataSize;
		Stream << ChunkInfo.DataSector;
		Stream << ChunkInfo.Guid;
		Stream << ChunkInfo.Hash;
		Stream << ChunkInfo.DataGroup;
		Stream.seek(1, CStream::Cur);
		Stream << ChunkInfo.Flags;
		Stream.seek(4, CStream::Cur);

		return Stream;
	}
};

// A full Fortnite (or maybe other game) installation
class InstallArchive {
public:
	// at offset 0
	InstallHeader Header;

	// at offset 256
	InstallRunlist FileRunlist;

	// at offset 512
	InstallRunlist ChunkPartRunlist;

	// at offset 768
	InstallRunlist ChunkInfoRunlist;

	// at offset 1024
	InstallRunlist ChunkDataRunlist;

	// at offset 2048
	InstallManifestData ManifestData;

	// at offset 4096
	InstallRunIndex RunIndex;

	// at offset 8192 (sector 16)
	// technical start of file data and all

	InstallArchive(fs::path Path);
	~InstallArchive();

	friend class CRunlistStream;

private:
	void InitializeParse();
	void InitializeCreate();

	void Resize(InstallRunlist& Runlist, int64_t NewSize);

	CFileStream Stream;
};