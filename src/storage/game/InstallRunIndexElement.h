#pragma once

#include "../../utils/stream/CStream.h"

namespace EGL3::Storage::Game {
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
}