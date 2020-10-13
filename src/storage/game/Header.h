#pragma once

#include "GameId.h"

namespace EGL3::Storage::Game {
	struct Header {
		uint32_t Magic;						// 'EGIA' aka 0x41494745
		uint32_t ArchiveVersion;			// Version of the install archive, currently 0
		char Game[40];						// Game name - example: Fortnite
		char VersionStringLong[64];			// Example: ++Fortnite+Release-13.30-CL-13884634-Windows
		char VersionStringHR[16];			// Human readable version - example: 13.30
		GameId GameNumeric;					// Should be some sort of enum (GameId)
		uint64_t VersionNumeric;			// Game specific, but a new update will always have a higher number (e.g. Fortnite's P4 CL)

		uint64_t RunIndexOffset;			// Offset to RunIndex (in bytes, from the beggining of the file)
		uint64_t RunIndexSize;				// Size of RunIndex (in bytes)

		uint64_t FileRunlistOffset;			// Offset to FileRunlist (in bytes, from the beggining of the file)
		uint64_t FileRunlistSize;			// Size of FileRunlist (in bytes)

		uint64_t ChunkPartRunlistOffset;	// Offset to ChunkPartRunlist (in bytes, from the beggining of the file)
		uint64_t ChunkPartRunlistSize;		// Size of ChunkPartRunlist (in bytes)

		uint64_t ChunkInfoRunlistOffset;	// Offset to ChunkInfoRunlist (in bytes, from the beggining of the file)
		uint64_t ChunkInfoRunlistSize;		// Size of ChunkInfoRunlist (in bytes)

		uint64_t ChunkDataRunlistOffset;	// Offset to ChunkDataRunlist (in bytes, from the beggining of the file)
		uint64_t ChunkDataRunlistSize;		// Size of ChunkDataRunlist (in bytes)

		uint64_t ManifestDataOffset;		// Offset to ManifestData (in bytes, from the beggining of the file)

		enum Constants {
			MAGIC = 0x41494745,

			VERSION_INITIAL = 0,

			VERSION_LATEST_PLUS_ONE,
			VERSION_LATEST = VERSION_LATEST_PLUS_ONE - 1
		};
	};
}