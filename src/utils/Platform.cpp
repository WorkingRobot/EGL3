#include "Platform.h"

#include "Format.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Utils::Platform {
    std::string GetOSVersionInternal() {
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

    const std::string& GetOSVersion() {
        static std::string OSVersion = GetOSVersionInternal();

        return OSVersion;
    }
}