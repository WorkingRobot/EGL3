#include "installerAppDlg.h"

#include "resource.h"

namespace EGL3::Installer {
    using WM_UPDATE_PROGRESS_DATA = std::tuple<Backend::InstallState, float, std::string>;
    constexpr auto WM_UPDATE_PROGRESS = WM_USER + 1;

    using WM_ERROR_PROGRESS_DATA = std::tuple<std::string>;
    constexpr auto WM_ERROR_PROGRESS = WM_USER + 2;

    CinstallerAppDlg::CinstallerAppDlg(CWnd* pParent) :
        CDialogEx(IDD_INSTALLERAPP_DIALOG, pParent),
        IconHandle(AfxGetApp()->LoadIcon(IDR_MAINFRAME)),
        CurrentState(DialogState::Welcome)
    {
        VERIFY(BrushBackground.CreateSolidBrush(0x001E1E1E));
    }

    BEGIN_MESSAGE_MAP(CinstallerAppDlg, CDialogEx)
        ON_WM_PAINT()
        ON_WM_QUERYDRAGICON()
        ON_WM_CTLCOLOR()
        ON_BN_CLICKED(IDNEXT, &CinstallerAppDlg::OnClickedNext)
        ON_BN_CLICKED(IDBACK, &CinstallerAppDlg::OnClickedBack)
        ON_BN_CLICKED(IDCANCEL, &CinstallerAppDlg::OnClickedCancel)
        ON_STN_CLICKED(IDC_STATIC_AGREEMENT_Y, &CinstallerAppDlg::OnClickedLicenseYes)
        ON_STN_CLICKED(IDC_STATIC_AGREEMENT_N, &CinstallerAppDlg::OnClickedLicenseNo)
        ON_BN_CLICKED(IDC_RADIO_AGREEMENT_Y, &CinstallerAppDlg::OnClickedLicenseYes)
        ON_BN_CLICKED(IDC_RADIO_AGREEMENT_N, &CinstallerAppDlg::OnClickedLicenseNo)
        ON_BN_CLICKED(IDC_INSTALL_BROWSE, &CinstallerAppDlg::OnClickedBrowse)
        ON_MESSAGE(WM_UPDATE_PROGRESS, &CinstallerAppDlg::OnProgressUpdate)
        ON_MESSAGE(WM_ERROR_PROGRESS, &CinstallerAppDlg::OnProgressError)
    END_MESSAGE_MAP()

    void CinstallerAppDlg::ChangeState(DialogState state) {
        UnloadCurrentState();
        CurrentState = state;
        LoadCurrentState();
    }

#define HIDE_DLG(id) GetDlgItem(id)->ShowWindow(SW_HIDE)
#define SHOW_DLG(id) GetDlgItem(id)->ShowWindow(SW_NORMAL)
#define TEXT_DLG(id, txt) GetDlgItem(id)->SetWindowText(txt)
#define ENBL_DLG(id) GetDlgItem(id)->EnableWindow(TRUE)
#define DSBL_DLG(id) GetDlgItem(id)->EnableWindow(FALSE)

    void CinstallerAppDlg::UnloadCurrentState() {
        switch (CurrentState)
        {
        case DialogState::Welcome:
            HIDE_DLG(IDC_STATIC_PICTURE);
            HIDE_DLG(IDC_STATIC_WELCOME_DESC);
            HIDE_DLG(IDC_STATIC_TITLE);
            SHOW_DLG(IDBACK);
            SHOW_DLG(IDCANCEL);
            break;
        case DialogState::License:
            HIDE_DLG(IDC_STATIC_SUBSECTION);
            HIDE_DLG(IDC_STATIC_ICON);
            HIDE_DLG(IDC_EDIT_LICENSE);
            HIDE_DLG(IDC_STATIC_DESCRIPTION);
            HIDE_DLG(IDC_RADIO_AGREEMENT_Y);
            HIDE_DLG(IDC_STATIC_AGREEMENT_Y);
            HIDE_DLG(IDC_RADIO_AGREEMENT_N);
            HIDE_DLG(IDC_STATIC_AGREEMENT_N);
            ENBL_DLG(IDNEXT);
            break;
        case DialogState::DestLocation:
            HIDE_DLG(IDC_STATIC_SUBSECTION);
            HIDE_DLG(IDC_STATIC_ICON);
            HIDE_DLG(IDC_STATIC_DESCRIPTION);
            HIDE_DLG(IDC_EDIT_BROWSE);
            HIDE_DLG(IDC_INSTALL_BROWSE);
            TEXT_DLG(IDNEXT, "Next >");
            break;
        case DialogState::Install:
            HIDE_DLG(IDC_STATIC_SUBSECTION);
            HIDE_DLG(IDC_STATIC_ICON);
            HIDE_DLG(IDC_STATIC_DESCRIPTION);
            HIDE_DLG(IDC_STATIC_PROGRESS);
            HIDE_DLG(IDC_PROGRESS);
            break;
        case DialogState::Complete:
        {
            Backend.LaunchProgram();
            DestroyWindow();
            break;
        }
        }
    }

    void CinstallerAppDlg::LoadCurrentState() {
        switch (CurrentState)
        {
        case DialogState::Welcome:
            SHOW_DLG(IDC_STATIC_PICTURE);
            SHOW_DLG(IDC_STATIC_WELCOME_DESC);
            SHOW_DLG(IDC_STATIC_TITLE);
            HIDE_DLG(IDBACK);
            break;
        case DialogState::License:
            SHOW_DLG(IDC_STATIC_SUBSECTION);
            SHOW_DLG(IDC_STATIC_ICON);
            SHOW_DLG(IDC_EDIT_LICENSE);
            SHOW_DLG(IDC_STATIC_DESCRIPTION);
            SHOW_DLG(IDC_RADIO_AGREEMENT_Y);
            SHOW_DLG(IDC_STATIC_AGREEMENT_Y);
            SHOW_DLG(IDC_RADIO_AGREEMENT_N);
            SHOW_DLG(IDC_STATIC_AGREEMENT_N);
            TEXT_DLG(IDC_STATIC_SUBSECTION, "License Agreement");
            TEXT_DLG(IDC_STATIC_DESCRIPTION, "In order to use EGL3, you must agree to the following license.");
            if (IsDlgButtonChecked(IDC_RADIO_AGREEMENT_Y) == BST_CHECKED) {
                ENBL_DLG(IDNEXT);
            }
            else {
                DSBL_DLG(IDNEXT);
            }
            break;
        case DialogState::DestLocation:
            SHOW_DLG(IDC_STATIC_SUBSECTION);
            SHOW_DLG(IDC_STATIC_ICON);
            SHOW_DLG(IDC_STATIC_DESCRIPTION);
            SHOW_DLG(IDC_EDIT_BROWSE);
            SHOW_DLG(IDC_INSTALL_BROWSE);
            TEXT_DLG(IDC_STATIC_SUBSECTION, "Select Install Location");
            TEXT_DLG(IDC_STATIC_DESCRIPTION, "Select the folder you want to install EGL3 to.");
            TEXT_DLG(IDC_EDIT_BROWSE, Backend.GetInstallPath().string().c_str());
            TEXT_DLG(IDNEXT, "Install");
            break;
        case DialogState::Install:
            SHOW_DLG(IDC_STATIC_SUBSECTION);
            SHOW_DLG(IDC_STATIC_ICON);
            SHOW_DLG(IDC_STATIC_DESCRIPTION);
            SHOW_DLG(IDC_STATIC_PROGRESS);
            SHOW_DLG(IDC_PROGRESS);
            TEXT_DLG(IDC_STATIC_SUBSECTION, "Installing");
            TEXT_DLG(IDC_STATIC_DESCRIPTION, "Installing EGL3 for you, please wait...");
            TEXT_DLG(IDNEXT, "Done");
            DSBL_DLG(IDNEXT);
            HIDE_DLG(IDBACK);
            HIDE_DLG(IDCANCEL);
            Backend.Start();
            break;
        case DialogState::Complete:
            SHOW_DLG(IDC_STATIC_SUBSECTION);
            SHOW_DLG(IDC_STATIC_ICON);
            SHOW_DLG(IDC_STATIC_DESCRIPTION);
            TEXT_DLG(IDC_STATIC_SUBSECTION, "Done");
            TEXT_DLG(IDC_STATIC_DESCRIPTION, "You're done! EGL3 will now start and initialize for you.");
            TEXT_DLG(IDNEXT, "Finish");
            break;
        }
    }

    std::string GetResourceText(int Id) {
        HRSRC ResourceHandle = FindResource(NULL, MAKEINTRESOURCE(Id), "TEXT");
        if (ResourceHandle == NULL) {
            return "";
        }

        DWORD ResourceSize = SizeofResource(NULL, ResourceHandle);
        if (ResourceSize == 0) {
            return "";
        }

        HGLOBAL MemoryHandle = LoadResource(NULL, ResourceHandle);
        if (MemoryHandle == NULL) {
            return "";
        }

        LPCSTR Ptr = (LPCSTR)LockResource(MemoryHandle);
        if (Ptr == NULL) {
            FreeResource(MemoryHandle);
            return "";
        }

        std::string Ret(Ptr, ResourceSize);

        UnlockResource(MemoryHandle);
        FreeResource(MemoryHandle);

        return Ret;
    }

    BOOL CinstallerAppDlg::OnInitDialog()
    {
        CDialogEx::OnInitDialog();

        SetIcon(IconHandle, TRUE);
        SetIcon(IconHandle, FALSE);

        {
            if (CFont* Font = GetFont()) {
                LOGFONT LogFont;
                if (Font->GetLogFont(&LogFont)) {
                    LogFont.lfHeight = 24;
                    LogFont.lfWeight = FW_BOLD;
                    FontTitle.CreateFontIndirect(&LogFont);
                }
            }
            GetDlgItem(IDC_STATIC_TITLE)->SetFont(&FontTitle);
            GetDlgItem(IDC_STATIC_SUBSECTION)->SetFont(&FontTitle);
        }

        CheckDlgButton(IDC_RADIO_AGREEMENT_N, BST_CHECKED);

        {
            if (CFont* Font = GetFont()) {
                LOGFONT LogFont;
                if (Font->GetLogFont(&LogFont)) {
                    LogFont.lfHeight = 16;
                    LogFont.lfWeight = FW_BLACK;
                    strcpy_s(LogFont.lfFaceName, "Consolas");
                    FontLicense.CreateFontIndirect(&LogFont);
                }
            }
            GetDlgItem(IDC_EDIT_LICENSE)->SetFont(&FontLicense);
        }

        GetDlgItem(IDC_EDIT_LICENSE)->SetWindowText(GetResourceText(IDR_LICENSE).c_str());

        ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS))->SetRange(0, 1000);

        {
            CPngImage img;
            img.Load(IDB_PNG1, AfxGetResourceHandle());
            ((CStatic*)GetDlgItem(IDC_STATIC_PICTURE))->SetBitmap(img);
        }

        {
            CPngImage img;
            img.Load(IDB_PNG2, AfxGetResourceHandle());
            ((CStatic*)GetDlgItem(IDC_STATIC_ICON))->SetBitmap(img);
        }

        Backend.OnProgressUpdate.Set([this](Backend::InstallState State, float Progress, const std::string& Text) {
            auto Ctx = new WM_UPDATE_PROGRESS_DATA(State, Progress, Text);
            PostMessage(WM_UPDATE_PROGRESS, 0, (LPARAM)Ctx);
        });

        Backend.OnError.Set([this](const std::string& Error) {
            auto Ctx = new WM_ERROR_PROGRESS_DATA(Error);
            PostMessage(WM_ERROR_PROGRESS, 0, (LPARAM)Ctx);
        });

        return TRUE;
    }

    void CinstallerAppDlg::OnPaint()
    {
        if (IsIconic()) {
            CPaintDC dc(this);

            SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

            // Center icon in client rectangle
            int cxIcon = GetSystemMetrics(SM_CXICON);
            int cyIcon = GetSystemMetrics(SM_CYICON);
            CRect rect;
            GetClientRect(&rect);
            int x = (rect.Width() - cxIcon + 1) / 2;
            int y = (rect.Height() - cyIcon + 1) / 2;

            // Draw the icon
            dc.DrawIcon(x, y, IconHandle);
        }
        else {
            CDialogEx::OnPaint();
        }
    }

    HCURSOR CinstallerAppDlg::OnQueryDragIcon() {
        return IconHandle;
    }

    HBRUSH CinstallerAppDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
    {
        switch (nCtlColor)
        {
        case CTLCOLOR_STATIC:
            // set text color, transparent back node then 
            pDC->SetTextColor(0x00D1C7C3);
            pDC->SetBkMode(TRANSPARENT);
            return (HBRUSH)BrushBackground;
        case CTLCOLOR_EDIT:
            return (HBRUSH)CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
        default:
            return (HBRUSH)BrushBackground;
        }
    }

    void CinstallerAppDlg::OnClickedNext() {
        ChangeState(DialogState((uint8_t)CurrentState + 1));
    }

    void CinstallerAppDlg::OnClickedBack() {
        ChangeState(DialogState((uint8_t)CurrentState - 1));
    }

    void CinstallerAppDlg::OnClickedCancel() {
        int Ret = MessageBox("EGL3 isn't installed yet.\n\nExit installation?", "Exit Setup", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (Ret == IDYES) {
            CDialogEx::OnCancel();
        }
    }

    void CinstallerAppDlg::OnClickedLicenseYes() {
        CheckDlgButton(IDC_RADIO_AGREEMENT_Y, BST_CHECKED);
        CheckDlgButton(IDC_RADIO_AGREEMENT_N, BST_UNCHECKED);
        ENBL_DLG(IDNEXT);
    }

    void CinstallerAppDlg::OnClickedLicenseNo() {
        CheckDlgButton(IDC_RADIO_AGREEMENT_N, BST_CHECKED);
        CheckDlgButton(IDC_RADIO_AGREEMENT_Y, BST_UNCHECKED);
        DSBL_DLG(IDNEXT);
    }

    int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
        if (uMsg == BFFM_INITIALIZED)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }
        return 0;
    }

    void CinstallerAppDlg::OnClickedBrowse() {
        std::filesystem::path NewPath;

        {
            BROWSEINFO Info{
                .hwndOwner = GetSafeHwnd(),
                .ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI,
                .lpfn = BrowseCallbackProc,
                .lParam = (LPARAM)Backend.GetInstallPath().string().c_str()
            };
            LPITEMIDLIST IDList = SHBrowseForFolder(&Info);
            if (!IDList) {
                return;
            }

            TCHAR Path[MAX_PATH];
            if (!SHGetPathFromIDList(IDList, Path)) {
                return;
            }
            NewPath = Path;

            IMalloc* Malloc;
            if (SUCCEEDED(SHGetMalloc(&Malloc)))
            {
                Malloc->Free(IDList);
                Malloc->Release();
            }
        }

        std::error_code Code;
        if (std::filesystem::is_directory(NewPath, Code)) {
            Backend.SetInstallPath(NewPath / "EGL3");
            GetDlgItem(IDC_EDIT_BROWSE)->SetWindowText(Backend.GetInstallPath().string().c_str());
        }
    }

    constexpr const char* StateToString(Backend::InstallState State) {
        switch (State)
        {
        case Backend::InstallState::Connecting:
            return "Connecting to epic.gl";
        case Backend::InstallState::StoppingService:
            return "Stopping service";
        case Backend::InstallState::Copying:
            return "Copying files";
        case Backend::InstallState::Registry:
            return "Updating registry";
        case Backend::InstallState::Shortcut:
            return "Creating shortcut";
        case Backend::InstallState::StartingService:
            return "Starting service";
        case Backend::InstallState::Done:
            return "Done";
        default:
            return "Unknown";
        }
    }

    LRESULT CinstallerAppDlg::OnProgressUpdate(WPARAM wParam, LPARAM lParam)
    {
        std::unique_ptr<WM_UPDATE_PROGRESS_DATA> Ctx((WM_UPDATE_PROGRESS_DATA*)lParam);
        const auto& [State, Progress, Text] = *Ctx;
        
        GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText(Text.empty() ? StateToString(State) : std::format("{} {}", StateToString(State), Text.c_str()).c_str());
        ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS))->SetPos(Progress * 1000);

        if (State == Backend::InstallState::Done) {
            ENBL_DLG(IDNEXT);
        }

        return 0;
    }

    LRESULT CinstallerAppDlg::OnProgressError(WPARAM wParam, LPARAM lParam)
    {
        std::unique_ptr<WM_ERROR_PROGRESS_DATA> Ctx((WM_ERROR_PROGRESS_DATA*)lParam);
        const auto& [Error] = *Ctx;

        GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText(Error.c_str());
        MessageBox(Error.c_str(), "EGL3 Error", MB_ICONERROR);

        DestroyWindow();

        return 0;
    }
}