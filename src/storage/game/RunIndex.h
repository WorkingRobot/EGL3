#pragma once

#include "../../utils/streams/Stream.h"
#include "RunIndexElement.h"

#include <vector>

namespace EGL3::Storage::Game {
	struct RunIndex {
		uint32_t ElementCount;
		uint32_t NextAvailableSector;
		std::vector<RunIndexElement> Elements;

		friend Stream& operator>>(Stream& Stream, RunIndex& RunIndex) {
			Stream >> RunIndex.ElementCount;
			Stream >> RunIndex.NextAvailableSector;
			for (uint32_t i = 0; i < RunIndex.ElementCount; ++i) {
				auto& Run = RunIndex.Elements.emplace_back();
				Stream >> Run;
			}

			return Stream;
		}

		friend Stream& operator<<(Stream& Stream, RunIndex& RunIndex) {
			Stream << RunIndex.ElementCount;
			Stream << RunIndex.NextAvailableSector;
			for (auto& Run : RunIndex.Elements) {
				Stream << Run;
			}

			return Stream;
		}
	};
}