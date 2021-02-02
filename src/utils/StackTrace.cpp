#include "StackTrace.h"

#include <sstream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dbghelp.h>

#pragma comment(lib, "Dbghelp.lib")

template<size_t Size, class... ArgsT>
__forceinline int sprintfcat_s(char (&Buffer)[Size], const char* Format, ArgsT&&... Args) {
    auto BufLen = strlen(Buffer);
    return sprintf_s(Buffer + BufLen, Size - BufLen, Format, std::forward<ArgsT>(Args)...);
}

__forceinline DWORD GetModuleFileNameWithStack(STACKFRAME64* Stack, LPSTR FileName, DWORD Size) {
    HMODULE hModule;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)(Stack->AddrPC.Offset), &hModule);

    if (hModule) {
        return GetModuleFileNameA(hModule, FileName, Size);
    }

    return 0;
}

__forceinline std::string GetStack(CONTEXT* Context)
{
    auto CurrentProcess = GetCurrentProcess();
    auto CurrentThread = GetCurrentThread();

    char SymbolBuffer[sizeof(SYMBOL_INFO) + (MAX_SYM_NAME - 1) * sizeof(TCHAR)];
    char ModuleName[256];
    PSYMBOL_INFO Symbol = (PSYMBOL_INFO)SymbolBuffer;
    Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    Symbol->MaxNameLen = MAX_SYM_NAME;

    IMAGEHLP_LINE64 Line{ .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };

    STACKFRAME64 Stack;
    memset(&Stack, 0, sizeof(STACKFRAME64));

    DWORD LineDisplacement;

#if !defined(_M_AMD64)
    Stack.AddrPC.Offset = (*ctx).Eip;
    Stack.AddrPC.Mode = AddrModeFlat;
    Stack.AddrStack.Offset = (*ctx).Esp;
    Stack.AddrStack.Mode = AddrModeFlat;
    Stack.AddrFrame.Offset = (*ctx).Ebp;
    Stack.AddrFrame.Mode = AddrModeFlat;
#endif

    SymInitialize(CurrentProcess, NULL, TRUE);

#if defined(_M_AMD64)
    constexpr static DWORD MachineType = IMAGE_FILE_MACHINE_AMD64;
#else
    constexpr static DWORD MachineType = IMAGE_FILE_MACHINE_I386;
#endif

    char lineBuffer[2048];
    std::ostringstream Ret;
    while (StackWalk64(MachineType, CurrentProcess, CurrentThread, &Stack, Context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
    {
        strcpy(lineBuffer, "");
        if (!GetModuleFileNameWithStack(&Stack, ModuleName, sizeof(ModuleName))) {
            strcpy_s(ModuleName, "Unknown");
        }
        sprintfcat_s(lineBuffer, "in %s", ModuleName);

        if (SymFromAddr(CurrentProcess, Stack.AddrPC.Offset, NULL, Symbol)) {
            sprintfcat_s(lineBuffer, " at %s (0x%0llX)", Symbol->Name, Symbol->Address);
        }

        if (SymGetLineFromAddr64(CurrentProcess, Stack.AddrPC.Offset, &LineDisplacement, &Line)) {
            sprintfcat_s(lineBuffer, " in %s:%lu (0x%0llX)", Line.FileName, Line.LineNumber, Line.Address);
        }

        Ret << lineBuffer << "\n";
    }

    return Ret.str();
}

namespace EGL3::Utils {
    std::string GetStackTrace()
    {
        CONTEXT Context;
        RtlCaptureContext(&Context);
        return GetStack(&Context);
    }
}
