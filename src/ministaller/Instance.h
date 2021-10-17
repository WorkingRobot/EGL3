#pragma once

#include "Backend/Registry.h"
#include "Page.h"

namespace EGL3::Installer {
    class Instance : public Window {
    public:
        static void Initialize();

        Instance();

        ~Instance();

        void Quit();

    private:
        void ChangePage(int Delta);

        INT_PTR DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void InitUI();

        HRESULT OleInitResult;
        LANGID UILanguage;
        HINSTANCE hInstance;
        HANDLE hIcon;

        Backend::Registry Registry;
        Page Pages[(uint8_t)Page::Type::Count];
        Page* Page;

        friend class Page;
    };

}