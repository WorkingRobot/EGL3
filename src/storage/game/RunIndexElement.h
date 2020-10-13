#pragma once

#include "RunlistId.h"

namespace EGL3::Storage::Game {
	struct RunIndexElement {
		uint32_t SectorCount;			// The number of sectors this element has
		RunlistId AllocationId;			// The id of the runlist that this element is allocated to
	};
}