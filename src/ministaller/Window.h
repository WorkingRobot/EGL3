#pragma once

#include "Language.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Installer {
    class Window {
    public:
        HWND GetHwnd() const noexcept;

        void Notify(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) const;

    protected:
        void SetItemText(int IDDlgItem, Language::Entry Entry);
        void SetItemText(int IDDlgItem, const char* Text);

        void SetItemTextResized(int IDDlgItem, Language::Entry Entry);
        void SetItemTextResized(int IDDlgItem, const char* Text);

        size_t GetItemText(int IDDlgItem, char* Text, int TextSize);
        template<int Size>
        inline size_t GetItemText(int IDDlgItem, char(&Text)[Size])
        {
            return GetItemText(IDDlgItem, Text, Size);
        }

        void HideItem(int IDDlgItem);
        void ShowItem(int IDDlgItem);

        void EnableItem(int IDDlgItem, bool Enable = true);

        void SetActiveItem(int IDDlgItem);

        int SendMessageBox(const char* Text, UINT Type);

        static INT_PTR HandleStaticBkColor(UINT uMsg, WPARAM wParam, LPARAM lParam);

        HWND hWnd;

        Language Language;
    };
}