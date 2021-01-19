#include "Random.h"

#include <random>

namespace EGL3::Utils {
    static std::mt19937 RandGenerator{ std::random_device{}() };

    // Split into different compilation unit so there aren't multiple random devices (one static for all)
    void GenerateRandomGuid(char Guid[16])
    {
        *(uint32_t*)(Guid + 0) = RandGenerator();
        *(uint32_t*)(Guid + 4) = RandGenerator();
        *(uint32_t*)(Guid + 8) = RandGenerator();
        *(uint32_t*)(Guid + 12) = RandGenerator();
    }

    uint32_t Random(uint32_t Min, uint32_t Max)
    {
        return std::uniform_int_distribution<uint32_t>(Min, Max)(RandGenerator);
    }
}
