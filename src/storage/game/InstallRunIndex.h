#pragma once

#include "../../utils/stream/CStream.h"
#include "InstallRunIndexElement.h"

#include <vector>

namespace EGL3::Storage::Game {
	struct InstallRunIndex {
		uint32_t ElementCount;
		uint32_t NextAvailableSector;
		std::vector<InstallRunIndexElement> Elements;

		friend CStream& operator>>(CStream& Stream, InstallRunIndex& RunIndex) {
			Stream >> RunIndex.ElementCount;
			Stream >> RunIndex.NextAvailableSector;
			for (uint32_t i = 0; i < RunIndex.ElementCount; ++i) {
				auto& Run = RunIndex.Elements.emplace_back();
				Stream >> Run;
			}

			return Stream;
		}

		friend CStream& operator<<(CStream& Stream, InstallRunIndex& RunIndex) {
			Stream << RunIndex.ElementCount;
			Stream << RunIndex.NextAvailableSector;
			for (auto& Run : RunIndex.Elements) {
				Stream << Run;
			}

			return Stream;
		}
	};
}