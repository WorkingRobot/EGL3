#include "srv/Service.h"
#include "srv/ServiceConfig.h"
#include "srv/ServiceControl.h"
#include "utils/Crc32.h"

using namespace EGL3::Service;

int main(int argc, char* argv[]) {
    if (argc > 1) {
        switch (EGL3::Utils::Crc32<true>(argv[1]))
        {
        case EGL3::Utils::Crc32("INSTALL"):
            RunInstall();
            break;
        case EGL3::Utils::Crc32("QUERY"):
            RunQuery();
            break;
        case EGL3::Utils::Crc32("DESCRIBE"):
            RunDescribe();
            break;
        case EGL3::Utils::Crc32("ENABLE"):
            RunEnable();
            break;
        case EGL3::Utils::Crc32("DISABLE"):
            RunDisable();
            break;
        case EGL3::Utils::Crc32("DELETE"):
            RunDelete();
            break;
        case EGL3::Utils::Crc32("START"):
            RunStart();
            break;
        case EGL3::Utils::Crc32("STOP"):
            RunStop();
            break;
        case EGL3::Utils::Crc32("DACL"):
            RunDacl();
            break;
        default:
            printf("unknown command\n");
            break;
        }
    }
    else {
        RunMain();
    }
}