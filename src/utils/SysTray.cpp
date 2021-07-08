#include "SysTray.h"

#include "Interop.h"
#include "Log.h"
#include "Random.h"

#include <shellapi.h>
#include <windowsx.h>

namespace EGL3::Utils {
    constexpr UINT CallbackMsg = WM_APP + 2;

    std::unordered_map<HWND, SysTray&> SysTrays;

    SysTray::SysTray(Gtk::Window& Window) :
        Hwnd(Interop::GetHwnd(Window))
    {
        EGL3_VERIFY(GetSysTray(Hwnd) == nullptr, "Tray already exists");
        SysTrays.emplace((HWND)Hwnd, *this);

        Utils::GenerateRandomGuid(Guid);

        OldWinProc = (WNDPROC)GetWindowLongPtr((HWND)Hwnd, GWLP_WNDPROC);
        SetWindowLongPtr((HWND)Hwnd, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)SysTray::WndProc);

        NOTIFYICONDATA Data{
            .cbSize = sizeof(Data),
            .hWnd = (HWND)Hwnd,
            .uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID,
            .uCallbackMessage = CallbackMsg,
            .hIcon = Interop::GetIcon(Window.get_icon()),
            .guidItem = *(GUID*)&Guid
        };
        strncpy_s(Data.szTip, Window.get_title().c_str(), _TRUNCATE);

        EGL3_ENSURE(Shell_NotifyIcon(NIM_ADD, &Data), LogLevel::Error, "Could not create icon");
        
        Data.uVersion = NOTIFYICON_VERSION_4;
        EGL3_ENSURE(Shell_NotifyIcon(NIM_SETVERSION, &Data), LogLevel::Error, "Could not set version");
    }

    SysTray::~SysTray()
    {
        NOTIFYICONDATA Data{
            .cbSize = sizeof(Data),
            .uFlags = NIF_GUID,
            .guidItem = *(GUID*)&Guid
        };

        EGL3_ENSURE(Shell_NotifyIcon(NIM_DELETE, &Data), LogLevel::Error, "Could not delete icon");

        SetWindowLongPtr((HWND)Hwnd, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)OldWinProc);
        SysTrays.erase((HWND)Hwnd);
    }

    void SysTray::ShowNotification()
    {
        NOTIFYICONDATA Data{
            .cbSize = sizeof(Data),
            .uFlags = NIF_GUID | NIF_INFO,
            .szInfo = "info data here",
            .szInfoTitle = "EGL3 info title",
            .dwInfoFlags = NIIF_WARNING,
            .guidItem = *(GUID*)&Guid,
        };

        EGL3_ENSURE(Shell_NotifyIcon(NIM_MODIFY, &Data), LogLevel::Error, "Could not display notification");
    }

    SysTray* EGL3::Utils::SysTray::GetSysTray(void* Hwnd)
    {
        auto Itr = SysTrays.find((HWND)Hwnd);
        if (Itr == SysTrays.end()) {
            return nullptr;
        }
        return &Itr->second;
    }

    LRESULT CALLBACK SysTray::WndProc(void* hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        auto TrayPtr = GetSysTray(hWnd);
        EGL3_VERIFY(TrayPtr, "Tray does not exist");

        auto& Tray = *TrayPtr;
        switch (msg)
        {
        case CallbackMsg:
            switch (LOWORD(lParam))
            {
            case NIN_SELECT:
                Tray.OnClicked(GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam));
                break;
            case WM_CONTEXTMENU:
                Tray.OnContextMenu(GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam));
                break;
            }
            return 0;
        }

        return CallWindowProc((WNDPROC)Tray.OldWinProc, (HWND)hWnd, msg, wParam, lParam);
    }
}