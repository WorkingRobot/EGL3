#pragma once

#include <stdint.h>
#include <vector>

class InstallHeader {
	uint32_t Magic;					// 'EGIA' aka 0x41494745
	uint32_t ArchiveVersion;		// Version of the install archive, currently 0
	char Game[40];					// Game name - example: Fortnite
	char VersionStringLong[64];		// Example: ++Fortnite+Release-13.30-CL-13884634-Windows
	char VersionStringHR[16];		// Human readable version - example: 13.30
	uint64_t GameNumeric;			// Should be some sort of enum
	uint64_t VersionNumeric;		// Game specific, but a new update will always have a higher number (e.g. Fortnite's P4 CL)
};

class InstallManifestData {
	char LaunchExeString[256];		// Path to the executable to launch
	char LaunchCommand[256];		// Extra command arguments to add to the executable
	char AppNameString[64];			// Manifest-specific app name - example: FortniteReleaseBuilds
	uint32_t AppID;					// Manifest-specific app id - example: 1
	uint32_t BaseUrlCount;			// Base urls (or CloudDirs), used to download chunks
	// A list of zero terminated strings (no alignment from here on out)
	// Example: ABCD\0DEFG\0 is a url count of 2 with ABCD and DEFG
};

class InstallRunlistElement {
	uint32_t SectorOffset;			// 1 sector = 512 bytes, this is the offset in sectors from the beginning of the install archive
	uint32_t SectorSize;			// Number of 512 byte sectors this run consists of
};

class InstallFile {
	char Filename[236];				// Engine/Binaries/ThirdParty/CEF3/Win64/chrome_elf.dll
	char SHA[20];					// SHA1 hash of the entire file
	int64_t ChunkPartDataOffset;	// Offset to the first chunk part (in chunk part data runlist)
	int64_t ChunkPartDataSize;		// Number of chunk part data in bytes, 8 byte aligned
	int64_t FileSize;				// Size of the file (calculated from chunk part data)
};

class InstallChunkPart {
	uint64_t Id;					// ID of the chunk, used to calculate the offset where it can be found in the chunk info runlist
	uint32_t Offset;				// Offset of the chunk data to start reading from
	uint32_t Size;					// Number of bytes of the chunk data to read from
};

class InstallChunkInfo {
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
};

// A full Fortnite (or maybe other game) installation
class InstallArchive {
	// at offset 0
	InstallHeader Header;

	// at offset 256
	std::vector<InstallRunlistElement> FileRunlist;

	// at offset 512
	std::vector<InstallRunlistElement> ChunkPartRunlist;

	// at offset 768
	std::vector<InstallRunlistElement> ChunkInfoRunlist;

	// at offset 1024
	std::vector<InstallRunlistElement> ChunkDataRunlist;

	// at offset 2048
	InstallManifestData ManifestData;

	// at offset 4096 (sector 8)
	// technical start of file data and all
};