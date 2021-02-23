#include "../utils/Crc32.h"
#include "Service.h"
#include "ServiceConfig.h"
#include "ServiceControl.h"

int main(int argc, char* argv[]) {
    if (argc > 1) {
        switch (EGL3::Utils::Crc32<true>(argv[1]))
        {
        case EGL3::Utils::Crc32("INSTALL"):
            EGL3SvcInstall();
            break;
        case EGL3::Utils::Crc32("QUERY"):
            EGL3SvcQuery();
            break;
        case EGL3::Utils::Crc32("DESCRIBE"):
            EGL3SvcDescribe();
            break;
        case EGL3::Utils::Crc32("ENABLE"):
            EGL3SvcEnable();
            break;
        case EGL3::Utils::Crc32("DISABLE"):
            EGL3SvcDisable();
            break;
        case EGL3::Utils::Crc32("DELETE"):
            EGL3SvcDelete();
            break;
        case EGL3::Utils::Crc32("START"):
            EGL3SvcStart();
            break;
        case EGL3::Utils::Crc32("STOP"):
            EGL3SvcStop();
            break;
        case EGL3::Utils::Crc32("DACL"):
            EGL3SvcDacl();
            break;
        default:
            printf("unknown command\n");
            break;
        }
    }
    else {
        EGL3SvcMain();
    }
}