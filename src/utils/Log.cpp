#include "Log.h"

#include <conio.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3 {
    bool Detail::ConsoleEnabled = false;

    struct CloseWaiter {
        ~CloseWaiter() {
            if (Detail::ConsoleEnabled) {
                printf("Press any key to exit...");
                _getch();
            }
        }
    };
    static CloseWaiter Waiter;

    void EnableConsole()
    {
        if (Detail::ConsoleEnabled) {
            return;
        }

        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

        auto StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (StdOutHandle == INVALID_HANDLE_VALUE) {
            return;
        }

        DWORD CurrentMode;
        if (!GetConsoleMode(StdOutHandle, &CurrentMode)) {
            return;
        }
        Detail::ConsoleEnabled = SetConsoleMode(StdOutHandle, CurrentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

}