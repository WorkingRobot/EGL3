#pragma once

#include <string>

namespace EGL3::Utils::Platform {
    constexpr const char* GetOSName() {
        return "Windows";
    }

    const std::string& GetOSVersion();

    struct MemoryConstants {
		uint64_t TotalPhysical;
		uint64_t TotalVirtual;
		size_t PageSize;
		size_t OsAllocationGranularity;
		size_t BinnedPageSize;
		size_t BinnedAllocationGranularity;
		uint64_t AddressLimit;
		uint32_t TotalPhysicalGB;
    };

    const MemoryConstants& GetMemoryConstants();
}