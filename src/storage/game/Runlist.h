#pragma once

#include "RunlistElement.h"
#include "RunlistId.h"

namespace EGL3::Storage::Game {
	struct Runlist {
		int64_t TotalSize;							// Total byte size of all the runs. Not required to be, but internal data can make it aligned.
		RunlistId AllocationId;						// Basically the id of the runlist (unique), used in the RunIndex
		uint32_t RunCount;							// Number of runs in the list below
		RunlistElement Runs[];						// List of runs showing where the data can be found

		bool GetRunIndex(uint64_t ByteOffset, uint32_t& RunIndex, uint32_t& RunByteOffset) const
		{
			for (RunIndex = 0; RunIndex < RunCount; ++RunIndex) {
				if (ByteOffset < Runs[RunIndex].SectorSize * 512ull) {
					RunByteOffset = ByteOffset;
					return true;
				}
				ByteOffset -= Runs[RunIndex].SectorSize * 512ull;
			}
			return false;
		}
	};
}