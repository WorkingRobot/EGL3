#include "InUse.h"
#include "Backend.h"

using namespace EGL3::Installer::Backend;

#include <psapi.h>

//NTSTATUS PrintProcessesUsingFile(PCWSTR FileName)
//{
//    NTSTATUS Status;
//
//    ULONG BufSize = 0x8000;
//    PVOID Buf;
//    while (true) {
//        Buf = Alloc(BufSize);
//        Status = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemModuleInformationEx, Buf, BufSize, &BufSize);
//        if (Status != STATUS_INFO_LENGTH_MISMATCH) {
//            break;
//        }
//        Free(Buf);
//        BufSize *= 2;
//    }
//
//    if (!NT_SUCCESS(Status)) {
//        Free(Buf);
//        return Status;
//    }
//
//    for (auto ModuleInfo = (PRTL_PROCESS_MODULE_INFORMATION_EX)Buf;; ModuleInfo = (PRTL_PROCESS_MODULE_INFORMATION_EX)((PBYTE)ModuleInfo + ModuleInfo->NextOffset)) {
//        Log(L"%s\n", ModuleInfo->BaseInfo.FullPathName);
//
//        if (ModuleInfo->NextOffset == 0) {
//            break;
//        }
//    }
//
//    Free(Buf);
//    return Status;
//}

int PrintModules(DWORD ProcID)
{
    if (ProcID == 42212) {
        Log(L"MODULE!\n");
    }
    HANDLE ProcHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcID);
    if (ProcHandle == NULL) {
        Log(L"fail open proc %u\n", GetLastError());
        return 1;
    }

    DWORD BytesNeeded;
    HMODULE ModHandles[1024];
    if (EnumProcessModulesEx(ProcHandle, ModHandles, sizeof(ModHandles), &BytesNeeded, LIST_MODULES_ALL) != 0) {
        wchar_t Name[MAX_PATH];
        for (DWORD Idx = 0; Idx < (BytesNeeded / sizeof(HMODULE)); ++Idx) {
            if (GetModuleFileNameExW(ProcHandle, ModHandles[Idx], Name, sizeof(Name) / sizeof(wchar_t)) != 0) {
                Log(L"%s\n", Name);
            }
        }
    }

    CloseHandle(ProcHandle);
    return 0;
}

NTSTATUS PrintProcessesUsingFile(PCWSTR FileName)
{
    DWORD BytesNeeded;
    DWORD ProcIds[1024];
    if (EnumProcesses(ProcIds, sizeof(ProcIds), &BytesNeeded) != 0) {
        for (DWORD Idx = 0; Idx < (BytesNeeded / sizeof(DWORD)); ++Idx) {
            PrintModules(ProcIds[Idx]);
            if (ProcIds[Idx] == 42212) {
                Sleep(30000);
                ExitProcess(0);
            }
        }
    }

    return 0;
}