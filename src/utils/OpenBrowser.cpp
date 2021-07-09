#include "OpenBrowser.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>

namespace EGL3::Utils {
    // Split into a seperate file so including this header doesn't include windows.h
    bool OpenInBrowser(const std::string& Url)
    {
        return (uint64_t)ShellExecuteA(NULL, NULL, Url.c_str(), NULL, NULL, SW_SHOW) > 32;
    }
}
