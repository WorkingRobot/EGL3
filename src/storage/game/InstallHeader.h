#pragma once

#include "../../utils/stream/CStream.h"

namespace EGL3::Storage::Game {
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
}