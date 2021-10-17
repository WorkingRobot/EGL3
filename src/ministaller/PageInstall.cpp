#include "Page.h"

#include "backend/Installer.h"
#include "Instance.h"
#include "resource.h"

#include <CommCtrl.h>
#include <shellapi.h>
#include <ShlObj_core.h>
#include <windowsx.h>

namespace EGL3::Installer {
    INT_PTR Page::PageInstall::Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_INITDIALOG) {
            ListHwnd = GetDlgItem(hWnd, IDC_LIST1);
            ProgHwnd = GetDlgItem(hWnd, IDC_PROGRESS);

            RECT ListRect;
            GetClientRect(ListHwnd, &ListRect);
            LVCOLUMN Column{
                .mask = LVCF_WIDTH,
                .fmt = 0,
                .cx = ListRect.right - GetSystemMetrics(SM_CXVSCROLL),
                .pszText = NULL,
                .cchTextMax = 0,
                .iSubItem = -1
            };
            ListView_InsertColumn(ListHwnd, 0, &Column);

            ListView_SetExtendedListViewStyleEx(ListHwnd, LVS_EX_LABELTIP, LVS_EX_LABELTIP);

            Page->SetItemText(IDC_SHOWDETAILS, Language::Entry::ShowDetailsInstall);
            Page->HideItem(IDC_LIST1);
            Page->ShowItem(IDC_SHOWDETAILS);
            Page->SetActiveItem(IDC_SHOWDETAILS);

            SendMessage(GetDlgItem(hWnd, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(0, UINT16_MAX));

            Inst->SetItemText(IDC_TITLE, Language::Entry::TitleInstall);
            Inst->SetItemText(IDC_DESCRIPTION, Language::Entry::DescriptionInstall);

            Inst->ShowItem(IDC_BACK);
            Inst->SetItemText(IDC_BACK, Language::Entry::Back);
            Inst->ShowItem(IDOK);
            Inst->SetItemText(IDOK, Language::Entry::Install);
            return FALSE;
        }

        if (uMsg == WM_NOTIFY_START) {
            PTHREAD_START_ROUTINE ThreadRoutine = [](LPVOID lpThreadParameter) -> DWORD {
                return ((PageInstall*)lpThreadParameter)->Install();
            };
            DWORD ThreadId;
            auto ThreadHandle = CreateThread(NULL, 0, ThreadRoutine, this, 0, &ThreadId);
            if (ThreadHandle == NULL) {

            }
            else {
                CloseHandle(ThreadHandle);
            }
        }

        if (uMsg == WM_IN_INSTALL_ABORT) {
            Inst->SetItemText(IDC_TITLE, Language::Entry::TitleInstallAborted);
            Inst->SetItemText(IDC_DESCRIPTION, Language::Entry::DescriptionInstallAborted);

            Inst->HideItem(IDC_BACK);
            Inst->HideItem(IDOK);
        }

        if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_SHOWDETAILS) {
            Page->HideItem(IDC_SHOWDETAILS);
            Page->ShowItem(IDC_LIST1);
            Page->SetActiveItem(IDC_LIST1);
        }

        if (uMsg == WM_CONTEXTMENU && wParam == (WPARAM)ListHwnd)
        {
            int ItemCount = ListView_GetItemCount(ListHwnd);
            if (ItemCount > 0)
            {
                POINT Point;
                if (lParam != (LPARAM)((INT_PTR)-1)) {
                    Point = {
                        .x = GET_X_LPARAM(lParam),
                        .y = GET_Y_LPARAM(lParam)
                    };
                }
                else {
                    RECT Rect;
                    GetWindowRect(ListHwnd, &Rect);
                    Point = {
                        .x = Rect.left,
                        .y = Rect.top
                    };
                }

                HMENU Menu = CreatePopupMenu();
                AppendMenu(Menu, MF_STRING, 1, "Copy Details");
                if (TrackPopupMenu(Menu, TPM_NONOTIFY | TPM_RETURNCMD, Point.x, Point.y, 0, Page->GetHwnd(), 0) == TRUE)
                {
                    int TotalSize = 1; // 1 is for the null terminator

                    char Buffer[1024];
                    LVITEM Item{
                        .pszText = Buffer,
                        .cchTextMax = sizeof(Buffer)
                    };
                    for (int Idx = 0; Idx < ItemCount; ++Idx) {
                        TotalSize += (int)SendMessage(ListHwnd, LVM_GETITEMTEXT, Idx, (LPARAM)&Item) + 2;
                    }

                    OpenClipboard(0);
                    EmptyClipboard();

                    HGLOBAL BufHandle = GlobalAlloc(GHND, TotalSize * sizeof(TCHAR));
                    auto Buf = (LPTSTR)GlobalLock(BufHandle);
                    for (int Idx = 0; Idx < ItemCount; ++Idx) {
                        Item.pszText = Buf;
                        Buf += SendMessage(ListHwnd, LVM_GETITEMTEXT, Idx, (LPARAM)&Item);
                        *Buf++ = '\r';
                        *Buf++ = '\n';
                    }
                    // Buffer is zeroed when allocated, so no need to explicitly set the null terminator
                    GlobalUnlock(BufHandle);
                    SetClipboardData(CF_TEXT, BufHandle);

                    CloseClipboard();
                }
            }
        }

        return HandleStaticBkColor(uMsg, wParam, lParam);
    }

    constexpr Language::Entry FromBackend(Backend::Installer::Error Error)
    {
        return Language::Entry((uint8_t)Language::Entry::BadReadInstallation - 1 + (uint8_t)Error);
    }

    constexpr Language::Entry FromBackend(Backend::Installer::Log Log)
    {
        return Language::Entry((uint8_t)Language::Entry::ExecutingCommandInstallation + (uint8_t)Log);
    }

    void CreateDirectories(std::string_view Directory)
    {
        char SubDir[1024]{};
        int LastDirPos = 0;

        while (LastDirPos < Directory.size()) {
            auto DirPos = Directory.find('\\', LastDirPos);
            if (DirPos++ == std::string_view::npos || DirPos == Directory.size()) {
                DirPos = Directory.size();
            }
            memcpy(SubDir + LastDirPos, Directory.data() + LastDirPos, DirPos - LastDirPos);
            CreateDirectory(SubDir, NULL);
            LastDirPos = DirPos;
        }
    }

    std::string_view TrimFile(std::string_view File)
    {
        auto Pos = File.find_last_of('\\');
        if (Pos != std::string_view::npos) {
            return File.substr(0, Pos);
        }
        return {};
    }

    DWORD Page::PageInstall::Install()
    {
        auto DirView = Inst->Pages[(int)Page::Type::Directory].Directory.Directory;
        CreateDirectories(DirView);
        if (SetCurrentDirectory(DirView.data()) == FALSE) {
            return GetLastError();
        }

        Page->Notify(WM_NULL);

        ArchiveStream::ErrorCallback ArchiveErrorCallback = [](void* Ctx, ArchiveStream::Error Error) -> void {
            return ((PageInstall*)Ctx)->OnError(ArchiveStream::GetErrorString(Error));
        };

        ArchiveStream Stream(ArchiveStream::Section::Data, ArchiveErrorCallback, this);

        Backend::Installer::ReadCallback ReadCallback = [](void* ReadCtx, void* Out, size_t Size) -> size_t {
            return ((ArchiveStream*)ReadCtx)->Read(Out, Size);
        };

        Backend::Installer::ProgressCallback ProgressCallback = [](void* Ctx, size_t Count, size_t Total) -> void {
            ((PageInstall*)Ctx)->OnProgress(Count, Total);
        };

        Backend::Installer::LogCallback LogCallback = [](void* Ctx, Backend::Installer::Log Log, va_list Args) -> void {
            ((PageInstall*)Ctx)->OnLogV(FromBackend(Log), Args);
        };

        Backend::Installer::ErrorCallback ErrorCallback = [](void* Ctx, Backend::Installer::Error Error, va_list Args) -> void {
            SendMessage(((PageInstall*)Ctx)->ProgHwnd, PBM_SETSTATE, PBST_ERROR, 0);
            ((PageInstall*)Ctx)->OnErrorV(FromBackend(Error), Args);
        };

        Backend::Installer Installer(&Stream, ReadCallback, this, ProgressCallback, LogCallback, ErrorCallback);

        return (DWORD)Installer.GetError();
    }

    void Page::PageInstall::LogV(const char* Format, va_list Args)
    {
        char Buffer[1024]{};
        wvsprintf(Buffer, Format, Args);
        Log(Buffer);
    }

    void Page::PageInstall::LogF(const char* Format, ...)
    {
        va_list Args;
        va_start(Args, Format);
        LogV(Format, Args);
        va_end(Args);
    }

    void Page::PageInstall::Log(const char* Text)
    {
        Page->SetItemText(IDC_INTROTEXT, Text);

        LVITEM Item{
            .mask = LVIF_TEXT,
            .iItem = ListView_GetItemCount(ListHwnd),
            .pszText = (char*)Text
        };
        SendMessage(ListHwnd, LVM_INSERTITEM, 0, (LPARAM)&Item);
        ListView_EnsureVisible(ListHwnd, Item.iItem, 0);
    }

    void Page::PageInstall::OnProgress(size_t Count, size_t Total)
    {
        SendMessage(ProgHwnd, PBM_SETPOS, Count * UINT16_MAX / Total, 0);
    }

    void Page::PageInstall::OnLogV(Language::Entry Entry, va_list Args)
    {
        LogV(Page->Language.GetEntry(Entry), Args);
    }

    void Page::PageInstall::OnLogF(Language::Entry Entry, ...)
    {
        va_list Args;
        va_start(Args, Entry);
        OnLogV(Entry, Args);
        va_end(Args);
    }

    void Page::PageInstall::OnLog(Language::Entry Entry)
    {
        Log(Page->Language.GetEntry(Entry));
    }

    void Page::PageInstall::OnErrorV(Language::Entry Entry, va_list Args)
    {
        LogV(Page->Language.GetEntry(Entry), Args);
        Page->Notify(WM_IN_INSTALL_ABORT);
    }

    void Page::PageInstall::OnErrorF(Language::Entry Entry, ...)
    {
        va_list Args;
        va_start(Args, Entry);
        OnErrorV(Entry, Args);
        va_end(Args);
    }

    void Page::PageInstall::OnError(Language::Entry Entry)
    {
        Log(Page->Language.GetEntry(Entry));
        Page->Notify(WM_IN_INSTALL_ABORT);
    }
}