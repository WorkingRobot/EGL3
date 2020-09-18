#pragma once

#include "../../utils/streams/Stream.h"

namespace EGL3::Storage::Game {
	using Utils::Streams::Stream;

	struct RunIndexElement {
		uint32_t SectorCount;			// The number of sectors this element has
		uint32_t AllocationId;			// The id of the runlist that this element is allocated to

		friend Stream& operator>>(Stream& Stream, RunIndexElement& RunIndexElement) {
			Stream >> RunIndexElement.SectorCount;
			Stream >> RunIndexElement.AllocationId;

			return Stream;
		}

		friend Stream& operator<<(Stream& Stream, RunIndexElement& RunIndexElement) {
			Stream << RunIndexElement.SectorCount;
			Stream << RunIndexElement.AllocationId;

			return Stream;
		}
	};
}