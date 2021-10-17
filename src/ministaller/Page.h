#pragma once

#include "Window.h"
#include "ArchiveStream.h"

#include <optional>
#include <string_view>

namespace EGL3::Installer {
    class Instance;

    class Page : public Window {
    public:
        enum class Type : uint8_t {
            Init,
            License,
            Directory,
            Install,
            Complete,

            Count
        };

        Page(Instance& Instance, Type Type = Type::Init);

        ~Page();

        Type GetType() const noexcept;

        int GetDialogId() const noexcept;

        void Destroy();

        INT_PTR PageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        Type PageType;

        struct PageBase {
            Page* Page;
            Instance* Inst;
        };

        struct PageLicense : public PageBase {
            INT_PTR Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

            DWORD StreamLicense(LPBYTE pbBuff, LONG cb, LONG* pcb);

            void OnError(ArchiveStream::Error Error);

            std::optional<ArchiveStream> Stream;
        };

        struct PageDirectory : public PageBase {
            PageDirectory();

            INT_PTR Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

            int BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam);

            char DirBuffer[MAX_PATH];
            std::string_view Directory;
        };

        struct PageInstall : public PageBase {
            INT_PTR Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

            DWORD Install();

            void LogV(const char* Format, va_list Args);

            void LogF(const char* Format, ...);

            void Log(const char* Text);

            void OnProgress(size_t Count, size_t Total);

            void OnLogV(Language::Entry Entry, va_list Args);

            void OnLogF(Language::Entry Entry, ...);

            void OnLog(Language::Entry Entry);

            void OnErrorV(Language::Entry Entry, va_list Args);

            void OnErrorF(Language::Entry Entry, ...);

            void OnError(Language::Entry Entry);

            HWND ListHwnd;
            HWND ProgHwnd;
        };

        union {
            PageBase Base;
            PageLicense License;
            PageDirectory Directory;
            PageInstall Install;
        };
    };
}