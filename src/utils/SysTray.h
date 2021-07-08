#pragma once

#include "Callback.h"

#include <gtkmm.h>
#include <vector>

namespace EGL3::Utils {
    class SysTray {
    public:
        SysTray(Gtk::Window& Window);

        ~SysTray();

        void ShowNotification();

        Callback<void(int X, int Y)> OnClicked;
        Callback<void(int X, int Y)> OnContextMenu;

    private:
        static SysTray* GetSysTray(void* Hwnd);

        static intptr_t _stdcall WndProc(void* hWnd, uint32_t msg, uintptr_t wParam, intptr_t lParam);

        void* Hwnd;
        void* OldWinProc;
        char Guid[16];
    };
}