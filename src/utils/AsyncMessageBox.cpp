#include "AsyncMessageBox.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

typedef int(WINAPI* FUNC_MESSAGEBOX)(HWND, LPCTSTR, LPCTSTR, UINT);
typedef void(WINAPI* FUNC_EXITPROCESS)(UINT);

struct INJDATA {
    FUNC_MESSAGEBOX fnMessageBox;
    FUNC_EXITPROCESS fnExitProcess;

    UINT uType;
    CHAR lpText[2048];
    CHAR lpCaption[256];
};

static DWORD WINAPI ThreadFunc(INJDATA* pData) {
    pData->fnMessageBox(NULL, pData->lpText, pData->lpCaption, pData->uType);
    pData->fnExitProcess(0);

    return 0;
}

namespace EGL3::Utils {
    void AsyncMessageBox(const char Text[2048], const char Title[256], uint32_t Type)
    {
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        static HMODULE hUser32 = LoadLibrary("user32");
        static HMODULE hKernel32 = LoadLibrary("kernel32");
        static FUNC_MESSAGEBOX fnMessageBox = hUser32 ? (FUNC_MESSAGEBOX)GetProcAddress(hUser32, "MessageBoxA") : NULL;
        static FUNC_EXITPROCESS fnExitProcess = hKernel32 ? (FUNC_EXITPROCESS)GetProcAddress(hKernel32, "ExitProcess") : NULL;
        INJDATA* RemoteData = NULL;
        LPVOID RemoteCode = NULL;
        HANDLE RemoteThread = NULL;

        if (CreateProcess(0, (LPSTR)"\"explorer.exe\"", 0, 0, 0, CREATE_SUSPENDED | IDLE_PRIORITY_CLASS, 0, 0, &si, &pi)) {
            __try {
                if (!fnMessageBox || !fnExitProcess) {
                    __leave;
                }

                INJDATA LocalData = {
                    fnMessageBox,
                    fnExitProcess,
                    Type
                };
                strcpy_s(LocalData.lpText, Text);
                strcpy_s(LocalData.lpCaption, Title);

                RemoteData = (INJDATA*)VirtualAllocEx(pi.hProcess, 0, sizeof(INJDATA), MEM_COMMIT, PAGE_READWRITE);
                if (!RemoteData) {
                    __leave;
                }
                WriteProcessMemory(pi.hProcess, RemoteData, &LocalData, sizeof(INJDATA), NULL);

                // 128 bytes is a generous estimate. This is probably really unsafe, but there is no easy way to get the size of a function
                // I could use something like `((LPBYTE)AfterThreadFunc - (LPBYTE)ThreadFunc))` but whole program optimization can switch the order of the functions
                RemoteCode = VirtualAllocEx(pi.hProcess, 0, 128, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
                if (!RemoteCode) {
                    __leave;
                }
                WriteProcessMemory(pi.hProcess, RemoteCode, &ThreadFunc, 128, NULL);

                RemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)RemoteCode, RemoteData, 0, NULL);
                if (!RemoteThread) {
                    __leave;
                }

                // WaitForSingleObject(RemoteThread, INFINITE); // This will wait for the thread to exit
            }
            __finally {
                if (RemoteThread) {
                    CloseHandle(RemoteThread);
                }

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
    }
}
