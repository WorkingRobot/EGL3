#include "../../utils/Version.h"
#include "../backend/Packer.h"

using namespace EGL3;
using namespace EGL3::Installer;

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("Not enough arguments\n");
        printf("%s <input path> <eglu output> [json output] [patch notes (pango)]\n", argv[0]);
        return 0;
    }

    Backend::Packer Packer(Utils::Version::GetAppVersion(), Utils::Version::GetShortAppVersion(),
        Utils::Version::GetMajorVersion(), Utils::Version::GetMinorVersion(), Utils::Version::GetPatchVersion(),
        Utils::Version::GetVersionNum());

    Packer.SetInputPath(argv[1]);

    if (!Packer.Pack(argv[2])) {
        printf("Failed to pack\n");
        return 0;
    }

    if (argc >= 5) {
        if (!Packer.ExportJson(argv[3], argv[4])) {
            printf("Failed to export json\n");
            return 0;
        }
    }

    printf("Successful");
    return 0;
}