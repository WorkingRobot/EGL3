#pragma once

#include "../../utils/stream/CStream.h"

namespace EGL3::Storage::Game {
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
}