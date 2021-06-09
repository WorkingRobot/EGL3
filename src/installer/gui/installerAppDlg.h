#pragma once

#include "framework.h"
#include "../backend/Installer.h"

#include <filesystem>
#include <optional>

namespace EGL3::Installer {
	enum class InstallState : uint8_t {
		Welcome,
		License,
		DestLocation,
		Install,
		Complete
	};

	class CinstallerAppDlg : public CDialogEx {
	public:
		CinstallerAppDlg(CWnd* pParent = nullptr);

	private:
		CBrush BrushBackground;
		CFont FontTitle;
		CFont FontLicense;
		HICON IconHandle;

		std::filesystem::path InstallPath;
		std::optional<Backend::Installer> Backend;
		InstallState CurrentState;

		void UnloadState();
		void LoadState();
		void ChangeState(InstallState state);
		void UpdateLicenseCheck();
		void RunInstaller();

		static bool LaunchUnelevated(const std::filesystem::path& Path);

		DECLARE_MESSAGE_MAP()
		BOOL OnInitDialog();
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		afx_msg void OnBnClickedNext();
		afx_msg void OnBnClickedBack();
		afx_msg void OnBnClickedCancel();
		afx_msg void OnStnClickedStaticAgreementN();
		afx_msg void OnStnClickedStaticAgreementY();
		afx_msg void OnBnClickedRadioAgreementY();
		afx_msg void OnBnClickedRadioAgreementN();
		afx_msg void OnBnClickedInstallBrowse();
		afx_msg LRESULT OnProgressUpdate(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnProgressError(WPARAM wParam, LPARAM lParam);
	};
}
