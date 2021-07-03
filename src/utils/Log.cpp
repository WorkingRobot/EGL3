#include "Log.h"

#include "Config.h"

#include <conio.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3 {
    constexpr bool UseConsole = true;

    bool Detail::ColorsEnabled = false;

    struct CloseWaiter {
        ~CloseWaiter() {
            if constexpr (UseConsole && HasUI) {
                printf("Press any key to exit...");
                _getch();
            }
        }
    };
    static CloseWaiter Waiter;

    void EnableConsole()
    {
        if constexpr (!HasUI) {
            return;
        }

        if constexpr (UseConsole) {
            if (Detail::ColorsEnabled) {
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

            Detail::ColorsEnabled = SetConsoleMode(StdOutHandle, CurrentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
        else { // Use file
            auto Path = Utils::Config::GetFolder() / "logs" / std::format("{:%Y-%m-%d_%H-%M-%S}.log", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));

            freopen_s((FILE**)stdout, Path.string().c_str(), "w", stdout);
        }
    }

}