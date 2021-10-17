#include "Page.h"

#include "Instance.h"
#include "resource.h"

#include <Richedit.h>
#include <shellapi.h>

namespace EGL3::Installer {
    INT_PTR Page::PageLicense::Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_INITDIALOG) {
            ArchiveStream::ErrorCallback ErrorCallback = [](void* Ctx, ArchiveStream::Error Error) -> void {
                return ((PageLicense*)Ctx)->OnError(Error);
            };
            Stream.emplace(ArchiveStream::Section::License, ErrorCallback, this);

            auto EditBox = GetDlgItem(hWnd, IDC_EDIT);
            Page->SetActiveItem(IDC_EDIT);

            SendMessage(EditBox, EM_AUTOURLDETECT, TRUE, 0);
            SendMessage(EditBox, EM_SETEVENTMASK, 0, ENM_LINK | ENM_KEYEVENTS);
            // SendMessage(EditBox, EM_EXLIMITTEXT, 0, lstrlen(LicenseText) * sizeof(TCHAR)); // Only used in SF_RTF (default is 32k)
            EDITSTREAMCALLBACK Callback = [](DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) -> DWORD {
                return ((PageLicense*)dwCookie)->StreamLicense(pbBuff, cb, pcb);
            };
            EDITSTREAM EditStream{
                .dwCookie = (DWORD_PTR)this,
                .pfnCallback = Callback
            };
            SendMessage(EditBox, EM_STREAMIN, SF_TEXT, (LPARAM)&EditStream);

            Inst->SetItemText(IDC_TITLE, Language::Entry::TitleLicense);
            Inst->SetItemText(IDC_DESCRIPTION, Language::Entry::DescriptionLicense);
            Page->SetItemText(IDC_TOPTEXT, Language::Entry::TopTextLicense);
            Page->SetItemText(IDC_INTROTEXT, Language::Entry::IntroTextLicense);

            Inst->HideItem(IDC_BACK);
            Inst->ShowItem(IDOK);
            Inst->SetItemText(IDOK, Language::Entry::Agree);
            Inst->EnableItem(IDOK);
            return FALSE;
        }

        if (uMsg == WM_NOTIFY) {
            auto EditBox = GetDlgItem(hWnd, IDC_EDIT);

            // Handle link clicks
            auto Link = (ENLINK*)lParam;
            if (Link->nmhdr.code == EN_LINK && Link->msg == WM_LBUTTONDOWN) {
                if (Link->chrg.cpMax - Link->chrg.cpMin < 1024 - 1) {
                    char Buf[1024]{};
                    TEXTRANGE Range{
                        .chrg = Link->chrg,
                        .lpstrText = Buf
                    };
                    SendMessage(EditBox, EM_GETTEXTRANGE, 0, (LPARAM)&Range);
                    SetCursor(LoadCursor(0, IDC_WAIT));

                    SHELLEXECUTEINFO Info{
                        .cbSize = sizeof(Info),
                        .fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT,
                        .hwnd = Inst->hWnd,
                        .lpVerb = "open",
                        .lpFile = Range.lpstrText,
                        .lpParameters = NULL,
                        .lpDirectory = NULL,
                        .nShow = SW_SHOWNORMAL
                    };
                    ShellExecuteEx(&Info);

                    SetCursor(LoadCursor(0, IDC_ARROW));
                }
            }

            auto MsgFilter = (MSGFILTER*)lParam;
            if (MsgFilter->nmhdr.code == EN_MSGFILTER && MsgFilter->msg == WM_KEYDOWN) {
                if (MsgFilter->wParam == VK_RETURN) {
                    SendMessage(Inst->hWnd, WM_COMMAND, IDOK, 0);
                }
                if (MsgFilter->wParam == VK_ESCAPE) {
                    SendMessage(Inst->hWnd, WM_CLOSE, 0, 0);
                }
                return 1;
            }
        }

        return HandleStaticBkColor(uMsg, wParam, lParam);
    }

    DWORD Page::PageLicense::StreamLicense(LPBYTE pbBuff, LONG cb, LONG* pcb)
    {
        *pcb = Stream->Read(pbBuff, cb);
        return 0;
    }

    void Page::PageLicense::OnError(ArchiveStream::Error Error)
    {
        Page->SendMessageBox(Page->Language.GetEntry(ArchiveStream::GetErrorString(Error)), MB_ICONERROR);
        Inst->Quit();
    }
}