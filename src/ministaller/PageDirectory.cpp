#include "Page.h"

#include "Instance.h"
#include "resource.h"

#include <algorithm>
#include <ShlObj_core.h>
#include <Shlwapi.h>

namespace EGL3::Installer {
    Page::PageDirectory::PageDirectory() :
        DirBuffer{}
    {
        
    }

    template<int Size>
    std::string_view AddTrailingSlash(char(&Text)[Size], int Length)
    {
        if (Length == 0 || Text[Length - 1] != '\\') {
            if (Length + 1 != Size) {
                Text[Length] = '\\';
                Text[Length + 1] = 0;
                return { Text, (size_t)Length + 1 };
            }
        }
        return { Text, (size_t)Length };
    }

    std::string_view TrimSlashToEnd(std::string_view Buffer, std::string_view& Directory) {
        auto Pos = Buffer.find_last_of('\\');
        if (Pos != std::string_view::npos) {
            Directory = Buffer.substr(0, Pos);
            Buffer.remove_prefix(Pos + 1);
        }
        else {
            Directory = {};
        }
        return Buffer;
    }

    bool IsValidPathSpecifier(std::string_view Buffer) {
        if (Buffer.size() < 2) {
            return false;
        }
        char DriveLetter = Buffer[0] | 0x20; // < Make lowercase
        return Buffer.starts_with("\\\\") || (DriveLetter >= 'a' && DriveLetter <= 'z' && Buffer[1] == ':');
    }

    std::string_view SkipRoot(std::string_view Dir)
    {
        if (Dir.size() >= 3 && Dir[1] == ':' && Dir[2] == '\\') {
            Dir.remove_prefix(3);
            return Dir;
        }
        if (Dir.starts_with("\\\\")) {
            Dir.remove_prefix(2);

            // Remove host and share name: `\\host\share\`
            for (int i = 0; i < 2; ++i) {
                auto Idx = Dir.find('\\');
                if (Idx == std::string_view::npos) {
                    return {};
                }
                Dir.remove_prefix(Idx + 1);
            }

            return Dir;
        }
        return {};
    }

    bool GetFileFindData(const char* Buffer, WIN32_FIND_DATA& FindData)
    {
        auto Handle = FindFirstFile(Buffer, &FindData);
        if (Handle != INVALID_HANDLE_VALUE) {
            FindClose(Handle);
            return true;
        }
        return false;
    }

    std::string_view ValidateFilename(std::string_view Dir)
    {
        constexpr std::string_view BadChars = "*?|<>\":";
        constexpr std::string_view UNCPrefix = R"(\\?\)";
        constexpr std::string_view BadSuffix = " \\";

        int SkipChars = 0;
        if (Dir.starts_with(UNCPrefix)) {
            SkipChars += 4;
        }
        if (IsValidPathSpecifier(Dir)) {
            SkipChars += 2;
        }

        auto EndItr = std::remove_copy_if(Dir.data() + SkipChars, Dir.data() + Dir.size(), (char*)Dir.data() + SkipChars, [BadChars](char Val) { return Val < 32 || BadChars.find(Val) != std::string_view::npos; });
        Dir.remove_suffix(Dir.size() - std::distance((char*)Dir.data(), EndItr));

        auto LastSize = Dir.find_last_not_of(BadSuffix);
        if (LastSize != std::string_view::npos) {
            Dir.remove_suffix(Dir.size() - LastSize - 1);
        }

        return Dir;
    }

    bool IsValidInstallPath(std::string_view Dir)
    {
        static constexpr bool AllowRootInstall = false;

        auto Root = SkipRoot(Dir);
        int DriveSize = Dir.size() - Root.size();

        if (Root.empty()) {
            return false;
        }

        Root = ValidateFilename(Root);

        if constexpr (!AllowRootInstall) {
            if (Root.empty() || Root.starts_with('\\')) {
                return false;
            }
        }

        while (Dir.size() > DriveSize) {
            WIN32_FIND_DATA FindData;
            if (GetFileFindData(Dir.data(), FindData) && !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                return false;
            }
            TrimSlashToEnd(Dir, Dir);
        }

        char TmpBuf[MAX_PATH]{};
        auto TmpBufSize = std::min(Dir.size(), (size_t)MAX_PATH - 1);
        memcpy(TmpBuf, Dir.data(), TmpBufSize);
        Dir = AddTrailingSlash(TmpBuf, TmpBufSize);
        if (GetFileAttributes(Dir.data()) == INVALID_FILE_ATTRIBUTES) {
            return false;
        }

        return true;
    }

    bool GetAvailableSpace(std::string_view Dir, uint64_t& BytesAvailable)
    {
        ULARGE_INTEGER Available;

        char TmpBuf[MAX_PATH]{};
        auto TmpBufSize = std::min(Dir.size(), (size_t)MAX_PATH - 1);
        memcpy(TmpBuf, Dir.data(), TmpBufSize);

        while (*TmpBuf) {
            AddTrailingSlash(TmpBuf, TmpBufSize);
            if (GetDiskFreeSpaceEx(TmpBuf, &Available, NULL, NULL)) {
                BytesAvailable = Available.QuadPart;
                return true;
            }

            std::string_view NewDirectory;
            TrimSlashToEnd({ TmpBuf, TmpBufSize }, NewDirectory);
            TmpBufSize = NewDirectory.size();
        }

        return false;
    }

    void SetSizeText(HWND hWnd, const char* Prefix, uint64_t Size)
    {
        static constexpr const char* Suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };

        int Frac = 0;
        int i = 0;
        while (Size >= 1024) {
            Frac = Size & 0x3FF;
            Size >>= 10;
            ++i;
        }
        char FracBuf[4]{};
        wsprintf(FracBuf, "%03d", (Frac * 1000) >> 10);
        for (char* p = FracBuf + 3; p > FracBuf || (p == FracBuf && i == 0); --p) {
            if (*p && *p != '0') {
                break;
            }
            *p = 0;
        }

        char Buf[32]{};
        if (*FracBuf) {
            wsprintf(Buf, "%s%d.%s %s", Prefix, (int)Size, FracBuf, Suffixes[i]);
        }
        else {
            wsprintf(Buf, "%s%d %s", Prefix, (int)Size, Suffixes[i]);
        }

        SetWindowText(hWnd, Buf);
    }

    INT_PTR Page::PageDirectory::Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_INITDIALOG) {
            if (Directory.empty()) {
                if (Inst->Registry.Get(Backend::Registry::Value::InstallLocation, DirBuffer)) {
                    Directory = DirBuffer;
                }
            }

            if (IsValidPathSpecifier(Directory) && !SkipRoot(Directory).empty()) {
                Directory = AddTrailingSlash(DirBuffer, Directory.size());
            }
            Page->SetItemText(IDC_DIR, DirBuffer);
            Page->SetItemText(IDC_BROWSE, Language::Entry::BrowseDirectory);
            Page->SetItemText(IDC_SELDIRTEXT, Language::Entry::BoxDirectory);
            Page->SetActiveItem(IDC_DIR);

            SHAutoComplete(GetDlgItem(hWnd, IDC_DIR), SHACF_FILESYSTEM);

            Inst->SetItemText(IDC_TITLE, Language::Entry::TitleDirectory);
            Inst->SetItemText(IDC_DESCRIPTION, Language::Entry::DescriptionDirectory);
            Page->SetItemText(IDC_INTROTEXT, Language::Entry::IntroTextDirectory);

            Inst->ShowItem(IDC_BACK);
            Inst->SetItemText(IDC_BACK, Language::Entry::Back);
            Inst->ShowItem(IDOK);
            Inst->SetItemText(IDOK, Language::Entry::Install);
            return FALSE;
        }

        if (uMsg == WM_NOTIFY_CLOSE_WARN) {
            Directory = { DirBuffer, Page->GetItemText(IDC_DIR, DirBuffer) };
            Directory = ValidateFilename(Directory);
        }

        if (uMsg == WM_COMMAND) {
            int Id = LOWORD(wParam);
            if (Id == IDC_DIR && HIWORD(wParam) == EN_CHANGE) {
                uMsg = WM_IN_UPDATEMSG;
            }
            if (Id == IDC_BROWSE) {
                BFFCALLBACK Callback = [](HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) -> int {
                    return ((PageDirectory*)lpData)->BrowseCallbackProc(hwnd, uMsg, lParam);
                };
                BROWSEINFO BrowseInfo{
                    .hwndOwner = Inst->GetHwnd(),
                    // .pszDisplayName = OutputDisplayName,
                    .lpszTitle = Inst->Language.GetEntry(Language::Entry::BrowseTextDirectory),
                    .ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE,
                    .lpfn = Callback,
                    .lParam = (LPARAM)this
                };
                LPITEMIDLIST IdList = SHBrowseForFolder(&BrowseInfo);
                if (IdList) {
                    CoTaskMemFree(IdList);
                    Directory = AddTrailingSlash(DirBuffer, lstrlen(DirBuffer));

                    // Add auto append stuff here

                    Page->SetItemText(IDC_DIR, DirBuffer);
                }
                else {
                    uMsg = WM_IN_UPDATEMSG;
                }
            }
        }

        if (uMsg == WM_IN_UPDATEMSG || uMsg == WM_NOTIFY_START) {
            bool HasError = false;

            Directory = { DirBuffer, Page->GetItemText(IDC_DIR, DirBuffer) };
            if (!IsValidInstallPath(Directory)) {
                HasError = true;
            }

            static constexpr uint64_t TotalSizeBytes = 1 << 24;

            uint64_t AvailableBytes;
            if (GetAvailableSpace(Directory, AvailableBytes)) {
                if (AvailableBytes < TotalSizeBytes) {
                    HasError = true;
                }
                SetSizeText(GetDlgItem(hWnd, IDC_SPACEAVAILABLE), Inst->Language.GetEntry(Language::Entry::SpaceAvailDirectory), AvailableBytes);
            }
            else {
                Page->SetItemText(IDC_SPACEAVAILABLE, "");
            }

            SetSizeText(GetDlgItem(hWnd, IDC_SPACEREQUIRED), Inst->Language.GetEntry(Language::Entry::SpaceReqDirectory), TotalSizeBytes);

            Inst->EnableItem(IDOK, !HasError);
        }

        return HandleStaticBkColor(uMsg, wParam, lParam);
    }

    int Page::PageDirectory::BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam)
    {
        if (uMsg == BFFM_INITIALIZED) {
            Page->GetItemText(IDC_DIR, DirBuffer);
            SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)1, (LPARAM)DirBuffer);
        }
        if (uMsg == BFFM_SELCHANGED) {
            SendMessage(hWnd, BFFM_ENABLEOK, 0, SHGetPathFromIDList((LPITEMIDLIST)lParam, (TCHAR*)DirBuffer));
        }

        return 0;
    }
}