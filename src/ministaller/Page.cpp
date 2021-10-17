#include "Page.h"

namespace std {
    [[noreturn]] void __cdecl _Xout_of_range(const char*) {}
}

namespace EGL3::Installer {
    Page::Page(Instance& Instance, Type Type) :
        PageType(Type)
    {
        switch (PageType)
        {
        case Type::License:
            License = PageLicense();
            break;
        case Type::Directory:
            Directory = PageDirectory();
            break;
        case Type::Install:
            Install = PageInstall();
            break;
        case Type::Complete:
            break;
        }

        Base = PageBase{ this, &Instance };
    }

    Page::Type Page::GetType() const noexcept
    {
        return PageType;
    }

    int Page::GetDialogId() const noexcept
    {
        switch (PageType)
        {
        case Type::License:
            return 2;
        case Type::Directory:
            return 3;
        case Type::Install:
            return 6;
        case Type::Complete:
            return 0;
        default:
            return 0;
        }
    }

    Page::~Page() noexcept
    {
        Destroy();
    }

    void Page::Destroy()
    {
        DestroyWindow(hWnd);

        switch (PageType)
        {
        case Type::License:
            License.~License();
            break;
        case Type::Directory:
            Directory.~Directory();
            break;
        case Type::Install:
            Install.~Install();
            break;
        case Type::Complete:
            break;
        }
    }

    INT_PTR Page::PageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_INITDIALOG) {
            Base.Page = this;
            this->hWnd = hWnd;
        }

        switch (PageType)
        {
        case Type::License:
            return License.Proc(hWnd, uMsg, wParam, lParam);
        case Type::Directory:
            return Directory.Proc(hWnd, uMsg, wParam, lParam);
        case Type::Install:
            return Install.Proc(hWnd, uMsg, wParam, lParam);
        case Type::Complete:
        default:
            return 0;
        }
    }
}