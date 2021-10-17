#include "Window.h"

#include "resource.h"

namespace EGL3::Installer {
    HWND Window::GetHwnd() const noexcept
    {
        return hWnd;
    }

    void Window::Notify(UINT uMsg, WPARAM wParam, LPARAM lParam) const
    {
        SendMessage(hWnd, uMsg, wParam, lParam);
    }

    void Window::SetItemText(int IDDlgItem, Language::Entry Entry)
    {
        SetItemText(IDDlgItem, Language.GetEntry(Entry));
    }

    void Window::SetItemText(int IDDlgItem, const char* Text)
    {
        SetDlgItemText(hWnd, IDDlgItem, Text);
    }

    void Window::SetItemTextResized(int IDDlgItem, Language::Entry Entry)
    {
        SetItemTextResized(IDDlgItem, Language.GetEntry(Entry));
    }

    void Window::SetItemTextResized(int IDDlgItem, const char* Text)
    {
        auto hWnd = GetDlgItem(this->hWnd, IDDlgItem);

        auto DC = GetDC(hWnd);
        auto Font = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
        auto OldFont = SelectObject(DC, Font);
        SIZE Size;
        GetTextExtentPoint32(DC, Text, lstrlen(Text), &Size);
        SelectObject(DC, OldFont);
        ReleaseDC(hWnd, DC);

        SetWindowPos(hWnd, 0, 0, 0, Size.cx + 4, Size.cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

        SetWindowText(hWnd, Text);
    }

    size_t Window::GetItemText(int IDDlgItem, char* Text, int TextSize)
    {
        return GetDlgItemText(hWnd, IDDlgItem, Text, TextSize);
    }

    void Window::HideItem(int IDDlgItem)
    {
        ShowWindow(GetDlgItem(hWnd, IDDlgItem), SW_HIDE);
    }

    void Window::ShowItem(int IDDlgItem)
    {
        ShowWindow(GetDlgItem(hWnd, IDDlgItem), SW_SHOW);
    }

    void Window::EnableItem(int IDDlgItem, bool Enable)
    {
        EnableWindow(GetDlgItem(hWnd, IDDlgItem), Enable);
    }

    void Window::SetActiveItem(int IDDlgItem)
    {
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hWnd, IDDlgItem), TRUE);
    }

    int Window::SendMessageBox(const char* Text, UINT Type)
    {
        MSGBOXPARAMS Params{
            .cbSize = sizeof(Params),
            .hwndOwner = hWnd,
            .hInstance = (HINSTANCE)GetModuleHandle(NULL),
            .lpszText = Text,
            .lpszCaption = Language.GetEntry(Language::Entry::Caption),
            .dwStyle = Type,
            .lpszIcon = MAKEINTRESOURCE(IDI_ICON)
        };
        return MessageBoxIndirect(&Params);
    }

    HBRUSH hbrBkgnd = NULL;
    INT_PTR Window::HandleStaticBkColor(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_CTLCOLORSTATIC) {
            if (hbrBkgnd == NULL) {
                hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
            }

            switch (GetDlgCtrlID((HWND)lParam))
            {
            case IDC_HEADER:
            case IDC_TITLE:
            case IDC_DESCRIPTION:
            case IDC_ICO:
                return (INT_PTR)hbrBkgnd;
            default:
                break;
            }
        }

        return 0;
    }
}