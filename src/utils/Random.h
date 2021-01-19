#pragma once

#include <iterator>
#include <limits>
#include <stdint.h>

namespace EGL3::Utils {
	void GenerateRandomGuid(char Guid[16]);

	uint32_t Random(uint32_t Min = 0, uint32_t Max = std::numeric_limits<uint32_t>::max());

	template<class Iter>
	static Iter RandomChoice(Iter Start, Iter End) {
		std::advance(Start, Random(0, std::distance(Start, End) - 1));
		return Start;
	}
}