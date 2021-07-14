#pragma once

#include "framework.h"
#include "resource.h"

namespace EGL3::Installer {
	class CinstallerApp : public CWinApp {
	public:
		CinstallerApp();

	public:
		virtual BOOL InitInstance();

		DECLARE_MESSAGE_MAP()
	};

	extern CinstallerApp theApp;
}
