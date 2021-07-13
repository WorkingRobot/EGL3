#pragma once

#include "framework.h"
#include "../backend/Installer.h"

#include <filesystem>
#include <optional>

namespace EGL3::Installer {
	enum class DialogState : uint8_t {
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
		void ChangeState(DialogState state);
		void UnloadCurrentState();
		void LoadCurrentState();

		CBrush BrushBackground;
		CFont FontTitle;
		CFont FontLicense;
		HICON IconHandle;

		Backend::Installer Backend;
		DialogState CurrentState;

		DECLARE_MESSAGE_MAP()
		BOOL OnInitDialog();
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		afx_msg void OnClickedNext();
		afx_msg void OnClickedBack();
		afx_msg void OnClickedCancel();
		afx_msg void OnClickedLicenseYes();
		afx_msg void OnClickedLicenseNo();
		afx_msg void OnClickedBrowse();
		afx_msg LRESULT OnProgressUpdate(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnProgressError(WPARAM wParam, LPARAM lParam);
	};
}
