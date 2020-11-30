#include "RandGuid.h"

#include <random>

namespace EGL3::Utils {
	// Split into different compilation unit so there aren't multiple random devices (one static for all)
	void GenerateRandomGuid(uint8_t Guid[16])
	{
		static std::mt19937 RandGenerator{ std::random_device{}() };
		*(uint32_t*)(Guid + 0) = RandGenerator();
		*(uint32_t*)(Guid + 4) = RandGenerator();
		*(uint32_t*)(Guid + 8) = RandGenerator();
		*(uint32_t*)(Guid + 12) = RandGenerator();
	}
}
