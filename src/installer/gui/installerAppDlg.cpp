#include "installerAppDlg.h"

#include "resource.h"

namespace EGL3::Installer {
	static UINT UWM_UPDATE_PROGRESS = RegisterWindowMessage("fns_Install Progress");
	static UINT UWM_ERROR_PROGRESS = RegisterWindowMessage("fns_Install Error");

	CinstallerAppDlg::CinstallerAppDlg(CWnd* pParent) :
		CDialogEx(IDD_INSTALLERAPP_DIALOG, pParent),
		IconHandle(AfxGetApp()->LoadIcon(IDR_MAINFRAME)),
		CurrentState(InstallState::Welcome)
	{
		VERIFY(BrushBackground.CreateSolidBrush(0x001E1E1E));
	}

	BEGIN_MESSAGE_MAP(CinstallerAppDlg, CDialogEx)
		ON_WM_PAINT()
		ON_WM_QUERYDRAGICON()
		ON_WM_CTLCOLOR()
		ON_BN_CLICKED(IDNEXT, &CinstallerAppDlg::OnBnClickedNext)
		ON_BN_CLICKED(IDBACK, &CinstallerAppDlg::OnBnClickedBack)
		ON_BN_CLICKED(IDCANCEL, &CinstallerAppDlg::OnBnClickedCancel)
		ON_STN_CLICKED(IDC_STATIC_AGREEMENT_N, &CinstallerAppDlg::OnStnClickedStaticAgreementN)
		ON_STN_CLICKED(IDC_STATIC_AGREEMENT_Y, &CinstallerAppDlg::OnStnClickedStaticAgreementY)
		ON_BN_CLICKED(IDC_RADIO_AGREEMENT_Y, &CinstallerAppDlg::OnBnClickedRadioAgreementY)
		ON_BN_CLICKED(IDC_RADIO_AGREEMENT_N, &CinstallerAppDlg::OnBnClickedRadioAgreementN)
		ON_BN_CLICKED(IDC_INSTALL_BROWSE, &CinstallerAppDlg::OnBnClickedInstallBrowse)
		ON_REGISTERED_MESSAGE(UWM_UPDATE_PROGRESS, &CinstallerAppDlg::OnProgressUpdate)
		ON_REGISTERED_MESSAGE(UWM_ERROR_PROGRESS, &CinstallerAppDlg::OnProgressError)
	END_MESSAGE_MAP()

	static std::string GetResourceText(int Id) {
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

		// Set the icon for this dialog.  The framework does this automatically
		// when the application's main window is not a dialog
		SetIcon(IconHandle, TRUE);			// Set big icon
		SetIcon(IconHandle, FALSE);		// Set small icon

		if (CFont* font = GetFont()) {
			LOGFONT lf;

			if (font->GetLogFont(&lf)) {
				lf.lfHeight = 24;
				lf.lfWeight = FW_BOLD;
				FontTitle.CreateFontIndirect(&lf);
			}
		}
		GetDlgItem(IDC_STATIC_TITLE)->SetFont(&FontTitle);
		GetDlgItem(IDC_STATIC_SUBSECTION)->SetFont(&FontTitle);
		CheckDlgButton(IDC_RADIO_AGREEMENT_N, BST_CHECKED);

		if (CFont* font = GetFont()) {
			LOGFONT lf;

			if (font->GetLogFont(&lf)) {
				lf.lfHeight = 16;
				strcpy_s(lf.lfFaceName, "Consolas");
				FontLicense.CreateFontIndirect(&lf);
			}
		}

		GetDlgItem(IDC_EDIT_LICENSE)->SetWindowText(GetResourceText(IDR_LICENSE).c_str());

		GetDlgItem(IDC_EDIT_LICENSE)->SetFont(&FontLicense);
		char* progFiles;
		if (_dupenv_s(&progFiles, nullptr, "PROGRAMFILES")) {
			// couldn't get the env variable
		}
		InstallPath = std::filesystem::path(progFiles) / "EGL3";

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

		return TRUE;
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


	// If you add a minimize button to your dialog, you will need the code below
	//  to draw the icon.  For MFC applications using the document/view model,
	//  this is automatically done for you by the framework.

	void CinstallerAppDlg::OnPaint()
	{
		if (IsIconic()) {
			CPaintDC dc(this); // device context for painting

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

	// The system calls this function to obtain the cursor to display while the user drags
	//  the minimized window.
	HCURSOR CinstallerAppDlg::OnQueryDragIcon() {
		return static_cast<HCURSOR>(IconHandle);
	}

	void CinstallerAppDlg::ChangeState(InstallState state) {
		UnloadState();
		CurrentState = state;
		LoadState();
	}

#define HIDE_DLG(id) GetDlgItem(id)->ShowWindow(SW_HIDE)
#define SHOW_DLG(id) GetDlgItem(id)->ShowWindow(SW_NORMAL)
#define TEXT_DLG(id, txt) GetDlgItem(id)->SetWindowText(txt)
#define ENBL_DLG(id) GetDlgItem(id)->EnableWindow(TRUE)
#define DSBL_DLG(id) GetDlgItem(id)->EnableWindow(FALSE)

	void CinstallerAppDlg::UnloadState() {
		switch (CurrentState)
		{
		case InstallState::Welcome:
			HIDE_DLG(IDC_STATIC_PICTURE);
			HIDE_DLG(IDC_STATIC_WELCOME_DESC);
			HIDE_DLG(IDC_STATIC_TITLE);
			SHOW_DLG(IDBACK);
			SHOW_DLG(IDCANCEL);
			break;
		case InstallState::License:
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
		case InstallState::DestLocation:
			HIDE_DLG(IDC_STATIC_SUBSECTION);
			HIDE_DLG(IDC_STATIC_ICON);
			HIDE_DLG(IDC_STATIC_DESCRIPTION);
			HIDE_DLG(IDC_EDIT_BROWSE);
			HIDE_DLG(IDC_INSTALL_BROWSE);
			TEXT_DLG(IDNEXT, "Next >");
			break;
		case InstallState::Install:
			HIDE_DLG(IDC_STATIC_SUBSECTION);
			HIDE_DLG(IDC_STATIC_ICON);
			HIDE_DLG(IDC_STATIC_DESCRIPTION);
			HIDE_DLG(IDC_STATIC_PROGRESS);
			HIDE_DLG(IDC_PROGRESS);
			break;
		case InstallState::Complete:
		{
			MessageBox(Backend->GetLaunchPath().string().c_str());
			ShellExecute(NULL, "open", Backend->GetLaunchPath().string().c_str(), NULL, NULL, SW_SHOWNORMAL);
			DestroyWindow();
			break;
		}
		}
	}

	void CinstallerAppDlg::LoadState() {
		switch (CurrentState)
		{
		case InstallState::Welcome:
			SHOW_DLG(IDC_STATIC_PICTURE);
			SHOW_DLG(IDC_STATIC_WELCOME_DESC);
			SHOW_DLG(IDC_STATIC_TITLE);
			HIDE_DLG(IDBACK);
			break;
		case InstallState::License:
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
		case InstallState::DestLocation:
			SHOW_DLG(IDC_STATIC_SUBSECTION);
			SHOW_DLG(IDC_STATIC_ICON);
			SHOW_DLG(IDC_STATIC_DESCRIPTION);
			SHOW_DLG(IDC_EDIT_BROWSE);
			SHOW_DLG(IDC_INSTALL_BROWSE);
			TEXT_DLG(IDC_STATIC_SUBSECTION, "Select Install Location");
			TEXT_DLG(IDC_STATIC_DESCRIPTION, "Select the folder you want to install EGL3 to.");
			TEXT_DLG(IDC_EDIT_BROWSE, InstallPath.string().c_str());
			TEXT_DLG(IDNEXT, "Install");
			break;
		case InstallState::Install:
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
			RunInstaller();
			break;
		case InstallState::Complete:
			SHOW_DLG(IDC_STATIC_SUBSECTION);
			SHOW_DLG(IDC_STATIC_ICON);
			SHOW_DLG(IDC_STATIC_DESCRIPTION);
			TEXT_DLG(IDC_STATIC_SUBSECTION, "Done");
			TEXT_DLG(IDC_STATIC_DESCRIPTION, "You're done! EGL3 will now start and initialize for you.");
			TEXT_DLG(IDNEXT, "Finish");
			break;
		}
	}

	void CinstallerAppDlg::OnBnClickedNext() {
		ChangeState((InstallState)((unsigned char)CurrentState + 1));
	}

	void CinstallerAppDlg::OnBnClickedBack() {
		ChangeState((InstallState)((unsigned char)CurrentState - 1));
	}

	void CinstallerAppDlg::OnBnClickedCancel() {
		int returnId = MessageBox("EGL3 isn't installed yet.\n\nExit installation?", "Exit Setup", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
		if (returnId == IDYES) {
			CDialogEx::OnCancel();
		}
	}


	void CinstallerAppDlg::OnStnClickedStaticAgreementN() {
		CheckDlgButton(IDC_RADIO_AGREEMENT_N, BST_CHECKED);
		CheckDlgButton(IDC_RADIO_AGREEMENT_Y, BST_UNCHECKED);
		UpdateLicenseCheck();
	}


	void CinstallerAppDlg::OnStnClickedStaticAgreementY() {
		CheckDlgButton(IDC_RADIO_AGREEMENT_Y, BST_CHECKED);
		CheckDlgButton(IDC_RADIO_AGREEMENT_N, BST_UNCHECKED);
		UpdateLicenseCheck();
	}

	void CinstallerAppDlg::UpdateLicenseCheck() {
		if (IsDlgButtonChecked(IDC_RADIO_AGREEMENT_Y) == BST_CHECKED) {
			ENBL_DLG(IDNEXT);
		}
		else {
			DSBL_DLG(IDNEXT);
		}
	}

	void CinstallerAppDlg::OnBnClickedRadioAgreementY() {
		UpdateLicenseCheck();
	}

	void CinstallerAppDlg::OnBnClickedRadioAgreementN() {
		UpdateLicenseCheck();
	}

	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
		if (uMsg == BFFM_INITIALIZED)
		{
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		}
		return 0;
	}

	void CinstallerAppDlg::OnBnClickedInstallBrowse() {
		std::filesystem::path fs_path;
		{
			BROWSEINFO a = { 0 };
			a.hwndOwner = GetSafeHwnd();
			a.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
			a.lpfn = BrowseCallbackProc;
			a.lParam = (LPARAM)InstallPath.c_str();
			LPITEMIDLIST pidl = SHBrowseForFolder(&a);
			if (!pidl) {
				return;
			}

			TCHAR path[MAX_PATH];
			SHGetPathFromIDList(pidl, path);
			fs_path = std::filesystem::path(path);

			IMalloc* imalloc;
			if (SUCCEEDED(SHGetMalloc(&imalloc)))
			{
				imalloc->Free(pidl);
				imalloc->Release();
			}
		}
		if (std::filesystem::is_directory(fs_path)) {
			InstallPath = fs_path / "EGL3";
		}
		GetDlgItem(IDC_EDIT_BROWSE)->SetWindowText(InstallPath.string().c_str());
	}

	LRESULT CinstallerAppDlg::OnProgressUpdate(WPARAM wParam, LPARAM lParam) {
		float Value;
		Backend::Unpacker::State State;
		Backend->GetProgress(Value, State);
		((CProgressCtrl*)GetDlgItem(IDC_PROGRESS))->SetPos(int(Value * 1000));
		LPCTSTR status;
		switch (State)
		{
		case Backend::Unpacker::State::Opening:
			GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText("Opening");
			break;
		case Backend::Unpacker::State::Copying:
			GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText("Copying files");
			break;
		case Backend::Unpacker::State::Registry:
			GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText("Updating registry");
			break;
		case Backend::Unpacker::State::Shortcut:
			GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText("Creating shortcut");
			break;
		case Backend::Unpacker::State::Done:
			GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText("Done");
			ENBL_DLG(IDNEXT);
			break;
		}
		return 0;
	}

	LRESULT CinstallerAppDlg::OnProgressError(WPARAM wParam, LPARAM lParam) {
		MessageBox((char*)lParam, "EGL3 Error", MB_ICONERROR);
		GetDlgItem(IDC_STATIC_PROGRESS)->SetWindowText((char*)lParam);
		delete[](char*)lParam;
		DestroyWindow();
		return 0;
	}

	void CinstallerAppDlg::RunInstaller() {
		((CProgressCtrl*)GetDlgItem(IDC_PROGRESS))->SetRange(0, 1000);

		Backend.emplace(InstallPath);
		Backend->OnProgressUpdate.Set([this]() {
			::PostMessage((HWND)GetSafeHwnd(), UWM_UPDATE_PROGRESS, 0, 0);
		});
		Backend->OnProgressError.Set([this](const std::string& Error) {
			char* err = new char[Error.size()];
			strcpy(err, Error.c_str());
			::PostMessage((HWND)GetSafeHwnd(), UWM_ERROR_PROGRESS, 0, (LPARAM)err);
		});

		Backend->Run();
	}
}