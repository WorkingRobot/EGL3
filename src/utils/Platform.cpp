#include "Platform.h"

#include "Format.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Utils::Platform {
    std::string GetOSVersionInternal()
    {
#if !_WIN64
#error "Program must be compiled for 64-bit Windows. 32-bit is not tested and is not intended."
#endif
        OSVERSIONINFOEX Info;
        Info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#pragma warning( push )
#pragma warning( disable : 28159 )
        if (GetVersionEx((LPOSVERSIONINFO)&Info)) {
#pragma warning( pop )
            return Utils::Format("%d.%d.%d.%d.%d.%s", Info.dwMajorVersion, Info.dwMinorVersion, Info.dwBuildNumber, Info.wProductType, Info.wSuiteMask, "64bit");
        }
    }

    const std::string& GetOSVersion()
    {
        static std::string OSVersion = GetOSVersionInternal();

        return OSVersion;
    }

    // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Core/Public/Windows/WindowsPlatformMath.h#L103
    __forceinline uint64_t CountLeadingZeros64(uint64_t Value)
    {
        unsigned long Log2;
        long Mask = -long(_BitScanReverse64(&Log2, Value) != 0);
        return ((63 - Log2) & Mask) | (64 & ~Mask);
    }

    __forceinline uint64_t CeilLogTwo64(uint64_t Arg)
    {
        int64_t Bitmask = ((int64_t)(CountLeadingZeros64(Arg) << 57)) >> 63;
        return (64 - CountLeadingZeros64(Arg - 1)) & (~Bitmask);
    }

    __forceinline uint64_t RoundUpToPowerOfTwo64(uint64_t Arg)
    {
        return uint64_t(1) << CeilLogTwo64(Arg);
    }

    // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformMemory.cpp#L256
    MemoryConstants GetMemoryConstantsInternal()
    {
        MEMORYSTATUSEX StatusEx;
        StatusEx.dwLength = sizeof(StatusEx);
        GlobalMemoryStatusEx(&StatusEx);

        SYSTEM_INFO SystemInfo;
        GetSystemInfo(&SystemInfo);

        MemoryConstants Constants{
            .TotalPhysical = StatusEx.ullTotalPhys,
            .TotalVirtual = StatusEx.ullTotalVirtual,
            .PageSize = SystemInfo.dwPageSize,
            .OsAllocationGranularity = SystemInfo.dwAllocationGranularity,
            .BinnedPageSize = SystemInfo.dwAllocationGranularity,
            .BinnedAllocationGranularity = SystemInfo.dwPageSize,
            .AddressLimit = RoundUpToPowerOfTwo64(Constants.TotalPhysical),
            .TotalPhysicalGB = (uint32_t)((Constants.TotalPhysical + 1024 * 1024 * 1024 - 1) / 1024 / 1024 / 1024),
        };

        return Constants;
    }

    const MemoryConstants& GetMemoryConstants()
    {
        static MemoryConstants Constants = GetMemoryConstantsInternal();

        return Constants;
    }
}