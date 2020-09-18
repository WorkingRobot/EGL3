#pragma once

#include "../../utils/stream/CStream.h"
#include "InstallRunlistElement.h"

#include <vector>

namespace EGL3::Storage::Game {
	struct InstallRunlist {
		int64_t TotalSize;							// Total byte size of all the runs. Not required to be, but internal data can make it aligned.
		uint32_t AllocationId;						// Basically the id of the runlist (unique), used in the RunIndex
		uint32_t RunCount;							// Number of runs in the list below
		std::vector<InstallRunlistElement> Runs;	// List of runs showing where the data can be found

		friend CStream& operator>>(CStream& Stream, InstallRunlist& Runlist) {
			Stream >> Runlist.TotalSize;
			Stream >> Runlist.AllocationId;
			Stream >> Runlist.RunCount;
			for (uint32_t i = 0; i < Runlist.RunCount; ++i) {
				auto& Run = Runlist.Runs.emplace_back();
				Stream >> Run;
			}

			return Stream;
		}

		friend CStream& operator<<(CStream& Stream, InstallRunlist& Runlist) {
			Stream << Runlist.TotalSize;
			Stream << Runlist.AllocationId;
			Stream << Runlist.RunCount;
			for (auto& Run : Runlist.Runs) {
				Stream << Run;
			}

			return Stream;
		}

		bool GetRunIndex(uint64_t ByteOffset, uint32_t& RunIndex, uint32_t& RunByteOffset) const
		{
			for (RunIndex = 0; RunIndex < Runs.size(); ++RunIndex) {
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