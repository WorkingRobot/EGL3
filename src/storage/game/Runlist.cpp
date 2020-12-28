#include "Runlist.h"

namespace EGL3::Storage::Game {
	bool Runlist::GetRunIndex(uint64_t ByteOffset, uint32_t& RunIndex, uint32_t& RunByteOffset) const
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
}