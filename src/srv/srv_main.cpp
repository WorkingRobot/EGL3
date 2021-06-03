#include "Service.h"
#include "ServiceConfig.h"
#include "ServiceControl.h"
#include "../utils/Crc32.h"

#include <conio.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>

bool IsProcessElevated() {
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    BOOL b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
    if (b) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
            b = false;
        }
        FreeSid(AdministratorsGroup);
    }

    return b;
}

using namespace EGL3;
using namespace EGL3::Service;

int main(int argc, char* argv[]) {
    if (argc > 1) {
        if (!IsProcessElevated()) {
            CHAR FilePath[MAX_PATH];
            if (!GetModuleFileName(NULL, FilePath, MAX_PATH)) {
                printf("Cannot launch as admin (%d)\n", GetLastError());
                return GetLastError();
            }

            std::string CommandArgs = argv[1];
            if (argc > 2) {
                CommandArgs += " ";
                CommandArgs += argv[2];
            }

            SHELLEXECUTEINFO Info{
                .cbSize = sizeof(Info),
                .fMask = SEE_MASK_NOCLOSEPROCESS,
                .hwnd = NULL,
                .lpVerb = "runas",
                .lpFile = FilePath,
                .lpParameters = CommandArgs.c_str(),
                .lpDirectory = NULL,
                .nShow = SW_NORMAL,
                .hInstApp = NULL
            };

            if (!ShellExecuteEx(&Info) || Info.hProcess == NULL) {
                printf("Cannot launch as admin (%d)\n", GetLastError());
                return GetLastError();
            }

            WaitForSingleObject(Info.hProcess, INFINITE);

            DWORD ReturnCode;
            if (!GetExitCodeProcess(Info.hProcess, &ReturnCode)) {
                ReturnCode = GetLastError();
            }

            CloseHandle(Info.hProcess);

            return ReturnCode;
        }

        int ReturnCode;
        switch (Utils::Crc32<true>(argv[1]))
        {
        case Utils::Crc32("PATCH"):
            // Bundle-like command, used by the main egl3 app
            RunInstall();
            RunDacl();
            RunDescribe();
            RunEnable();
            ReturnCode = RunStart();
            break;
        case Utils::Crc32("INSTALL"):
            ReturnCode = RunInstall();
            break;
        case Utils::Crc32("QUERY"):
            ReturnCode = RunQuery();
            break;
        case Utils::Crc32("DESCRIBE"):
            ReturnCode = RunDescribe();
            break;
        case Utils::Crc32("ENABLE"):
            ReturnCode = RunEnable();
            break;
        case Utils::Crc32("DISABLE"):
            ReturnCode = RunDisable();
            break;
        case Utils::Crc32("DELETE"):
            RunStop();
            ReturnCode = RunDelete();
            break;
        case Utils::Crc32("START"):
            ReturnCode = RunStart();
            break;
        case Utils::Crc32("STOP"):
            ReturnCode = RunStop();
            break;
        case Utils::Crc32("DACL"):
            ReturnCode = RunDacl();
            break;
        case Utils::Crc32("USERMODE"):
            ReturnCode = RunUsermode();
            break;
        default:
            printf("unknown command\n");
            return ERROR_INVALID_PARAMETER;
        }
        if (!(argc > 2 && Utils::Crc32<true>(argv[2]) == Utils::Crc32("NOWAIT"))) {
            printf("\nPress any key to continue...");
#pragma warning( push )
#pragma warning( disable : 6031 )
            _getch();
#pragma warning( pop )
        }
        return ReturnCode;
    }
    else {
        RunMain();
    }
}