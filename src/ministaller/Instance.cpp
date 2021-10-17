#include "Instance.h"

#include "resource.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <CommCtrl.h>
#include <Ole2.h>
#include <Richedit.h>

namespace EGL3::Installer {
    void Instance::Initialize()
    {
        SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);

        INITCOMMONCONTROLSEX InitCtrls{
            .dwSize = sizeof(InitCtrls),
            .dwICC = ICC_WIN95_CLASSES
        };
        InitCommonControlsEx(&InitCtrls);

        if (!LoadLibrary("RichEd20")) {
            LoadLibrary("RichEd32");
        }

        WNDCLASS Class;
        if (!GetClassInfo(NULL, RICHEDIT_CLASS, &Class)) {
            GetClassInfo(NULL, RICHEDIT_CLASS10A, &Class);
            Class.lpszClassName = RICHEDIT_CLASS;
            RegisterClass(&Class);
        }
    }

    Instance* _this = nullptr;
    Instance::Instance() :
        OleInitResult(OleInitialize(NULL)),
        UILanguage(GetUserDefaultUILanguage()),
        hInstance((HINSTANCE)GetModuleHandle(NULL)),
        hIcon(LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED)),
        Pages{ Installer::Page(*this), Installer::Page(*this, Page::Type::License), Installer::Page(*this, Page::Type::Directory), Installer::Page(*this, Page::Type::Install), Installer::Page(*this, Page::Type::Complete) },
        Page(&Pages[0])
    {
        if (_this == nullptr) {
            _this = this;
        }

        DLGPROC DlgProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> INT_PTR {
            return _this->DlgProc(hWnd, uMsg, wParam, lParam);
        };
        auto BoxRet = DialogBox(hInstance, MAKEINTRESOURCE(IDD_INST), 0, DlgProc);
    }

    Instance::~Instance()
    {
        OleUninitialize();

        if (_this == this) {
            _this = nullptr;
        }
    }

    void Instance::Quit()
    {
        EndDialog(hWnd, 0);
    }

    void Instance::ChangePage(int Delta)
    {
        SendMessage(hWnd, WM_NOTIFY_OUTER_NEXT, (WPARAM)Delta, 0);
    }

    INT_PTR Instance::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_INITDIALOG || uMsg == WM_NOTIFY_OUTER_NEXT) {
            int Delta = (int)wParam;

            if (uMsg == WM_INITDIALOG) {
                this->hWnd = hWnd;

                InitUI();

                Delta = 1;
            }

            Page->Notify(WM_NOTIFY_CLOSE_WARN);
            Page->Destroy();

            // 0 means the window should close (or else this message would be futile)
            if (Delta == 0) {
                Quit();
            }
            else {
                Page = &Pages[(int)Page->GetType() + Delta];

                DLGPROC PageProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> INT_PTR {
                    return _this->Page->PageProc(hWnd, uMsg, wParam, lParam);
                };
                CreateDialogParam(hInstance, MAKEINTRESOURCE(100 + Page->GetDialogId()), hWnd, PageProc, (LPARAM)&Page);
                
                RECT Rect;
                GetWindowRect(GetDlgItem(hWnd, IDC_CHILDRECT), &Rect);
                ScreenToClient(hWnd, (LPPOINT)&Rect);
                SetWindowPos(Page->GetHwnd(), 0, Rect.left, Rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
                ShowWindow(Page->GetHwnd(), SW_SHOWNA);
                Page->Notify(WM_NOTIFY_START);
            }

            return FALSE;
        }

        if (uMsg == WM_SIZE) {
            if (wParam == SIZE_MAXIMIZED) {
                static constexpr DWORD Mask = WS_MAXIMIZEBOX | WS_MAXIMIZE | WS_MINIMIZE;
                DWORD Style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
                if ((Style & Mask) == WS_MAXIMIZE) {
                    ShowWindow(hWnd, SW_SHOWNOACTIVATE);
                }
            }
        }

        if (uMsg == WM_QUERYENDSESSION) {
            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, FALSE);
            return TRUE;
        }

        if (uMsg == WM_COMMAND) {
            int id = LOWORD(wParam);
            HWND hCtl = GetDlgItem(hWnd, id);
            if (hCtl)
            {
                SendMessage(hCtl, BM_SETSTATE, FALSE, 0);
                if (!IsWindowEnabled(hCtl)) {
                    return 0;
                }
            }

            if (id == IDOK) {
                ChangePage(1);
            }
            else if (id == IDC_BACK) {
                ChangePage(-1);
            }
            else if (id == IDCANCEL) {
                ChangePage(0);
            }
            else
            {
                // Forward WM_COMMANDs to inner dialogs, can be custom ones.
                // Without this, enter on buttons in inner dialogs won't work.
                Page->Notify(WM_COMMAND, wParam, lParam);
            }
        }

        return HandleStaticBkColor(uMsg, wParam, lParam);
    }

    void Instance::InitUI()
    {
        {
            auto Title = GetDlgItem(hWnd, IDC_TITLE);
            auto Font = (HFONT)SendMessage(Title, WM_GETFONT, 0, 0);
            LOGFONT LFont{};
            GetObject(Font, sizeof(LOGFONT), &LFont);
            LFont.lfWeight = 700;
            SendMessage(Title, WM_SETFONT, (WPARAM)CreateFontIndirectA(&LFont), FALSE);
        }

        SetItemTextResized(IDC_VERSTR, Language::Entry::Branding);
        SetItemText(IDCANCEL, Language::Entry::Cancel);

        SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR)hIcon);
    }
}