#pragma once

#include "../../utils/streams/Stream.h"

namespace EGL3::Storage::Game {
	using Utils::Streams::Stream;

	struct RunlistElement {
		uint32_t SectorOffset;			// 1 sector = 512 bytes, this is the offset in sectors from the beginning of the install archive
		uint32_t SectorSize;			// Number of 512 byte sectors this run consists of

		friend Stream& operator>>(Stream& Stream, RunlistElement& RunlistElement) {
			Stream >> RunlistElement.SectorOffset;
			Stream >> RunlistElement.SectorSize;

			return Stream;
		}

		friend Stream& operator<<(Stream& Stream, RunlistElement& RunlistElement) {
			Stream << RunlistElement.SectorOffset;
			Stream << RunlistElement.SectorSize;

			return Stream;
		}
	};
}