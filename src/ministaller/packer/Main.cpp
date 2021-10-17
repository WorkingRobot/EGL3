#include "../backend/Packer.h"

using namespace EGL3::Installer::Backend;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Msi.h>

int Main() {
    /*static constexpr const char* UpgradeCode = "{F7F63D2E-309B-485F-A3DC-1AAE26E31B2B}";
    char GUID[39];
    for (DWORD Idx = 0; MsiEnumRelatedProducts(UpgradeCode, 0, Idx, GUID) == ERROR_SUCCESS; ++Idx) {
        Log("%s\n", GUID);
    }
    return 0;*/

    Packer Packer("out.egli", 15);
    Packer.Initialize();
    Packer.AddCommand("umodel");
    Packer.AddDirectory(R"(J:\Code\Visual Studio 2017\Projects\EGL3\vcpkg)");
    Packer.AddRegistry("EGL3", "WorkingRobot", "https://epic.gl/discord", "https://epic.gl/changelog", "https://epic.gl", "3.1");
    return 0;
}
