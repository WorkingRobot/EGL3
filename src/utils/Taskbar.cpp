#include "Taskbar.h"

#include "Assert.h"

// This #define is for gdk_win32_pixbuf_to_hicon_libgtk_only
// Sorry for the hacky solutions, but they're too useful to just not use
#define GDK_COMPILATION
#include <gdk/gdkwin32.h>

#include <ShObjIdl.h>
#include "..\modules\Taskbar.h"

namespace EGL3::Utils {
    Taskbar::Taskbar() {
        HRESULT Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        EGL3_CONDITIONAL_LOG(Result != CO_E_NOTINITIALIZED, LogLevel::Critical, "COM couldn't be initialized");
        CoInitialized = true;
        EGL3_CONDITIONAL_LOG(Result != RPC_E_CHANGED_MODE, LogLevel::Error, "COM has changed mode, this may be problematic");

        Result = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (LPVOID*)&TaskbarImpl);
        EGL3_CONDITIONAL_LOG(Result == S_OK, LogLevel::Critical, "Could not get taskbar COM interface");
    }

    Taskbar::~Taskbar() {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->Release();
        }
        CoUninitialize();
    }

    HWND GetHwnd(Gtk::Window& Window) {
        GdkWindow* InternalWindow = Window.get_window()->gobj();
        EGL3_CONDITIONAL_LOG(gdk_win32_window_is_win32(InternalWindow), LogLevel::Critical, "Window is not a Win32 window");

        return gdk_win32_window_get_impl_hwnd(InternalWindow);
    }

    HICON GetIcon(Gdk::Pixbuf& Pixbuf) {
        return gdk_win32_pixbuf_to_hicon_libgtk_only(Pixbuf.gobj());
    }

    void Taskbar::SetProgressValue(Gtk::Window& Window, uint64_t Value, uint64_t Total)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetProgressValue(GetHwnd(Window), Value, Total);
        }
    }

    void Taskbar::SetProgressState(Gtk::Window& Window, ProgressState NewState)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetProgressState(GetHwnd(Window), (TBPFLAG)NewState);
        }
    }

    std::unique_ptr<THUMBBUTTON[]> ConvertButtons(const std::vector<Taskbar::ThumbButton>& Buttons) {
        auto ButtonsImpl = std::make_unique<THUMBBUTTON[]>(Buttons.size());
        uint32_t Idx = 0;
        for (auto& Button : Buttons) {
            auto& ButtonImpl = ButtonsImpl[Idx++];

            ButtonImpl.iId = Button.Id;

            if (Button.Icon) {
                ButtonImpl.dwMask |= THB_ICON;
                ButtonImpl.hIcon = GetIcon(*Button.Icon);
            }

            if (!Button.Tooltip.empty()) {
                ButtonImpl.dwMask |= THB_TOOLTIP;
                wcscpy_s(ButtonImpl.szTip, Button.Tooltip.c_str());
            }

            if ((uint8_t)Button.Flags & (uint8_t)Taskbar::ThumbButtonFlags::InUse) {
                ButtonImpl.dwMask |= THB_FLAGS;
                // The xor is to remove the "InUse" flag
                ButtonImpl.dwFlags = (THUMBBUTTONFLAGS)Button.Flags ^ (THUMBBUTTONFLAGS)Taskbar::ThumbButtonFlags::InUse;
            }
        }

        return ButtonsImpl;
    }

    void Taskbar::AddThumbbarButtons(Gtk::Window& Window, const std::vector<ThumbButton>& Buttons)
    {
        if (TaskbarImpl) {
            auto ButtonsImpl = ConvertButtons(Buttons);

            ((ITaskbarList3*)TaskbarImpl)->ThumbBarAddButtons(GetHwnd(Window), (uint32_t)Buttons.size(), ButtonsImpl.get());
        }
    }

    void Taskbar::UpdateThumbbarButtons(Gtk::Window& Window, const std::vector<ThumbButton>& Buttons)
    {
        if (TaskbarImpl) {
            auto ButtonsImpl = ConvertButtons(Buttons);

            ((ITaskbarList3*)TaskbarImpl)->ThumbBarUpdateButtons(GetHwnd(Window), (uint32_t)Buttons.size(), ButtonsImpl.get());
        }
    }

    void Taskbar::SetOverlayIcon(Gtk::Window& Window, Gdk::Pixbuf& Icon)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetOverlayIcon(GetHwnd(Window), GetIcon(Icon), NULL);
        }
    }

    void Taskbar::SetOverlayIcon(Gtk::Window& Window, Gdk::Pixbuf& Icon, const std::wstring& Description)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetOverlayIcon(GetHwnd(Window), GetIcon(Icon), Description.c_str());
        }
    }

    void Taskbar::RemoveOverlayIcon(Gtk::Window& Window)
    {
        if (TaskbarImpl) {
#pragma warning( push )
#pragma warning( disable : 6387 )
            ((ITaskbarList3*)TaskbarImpl)->SetOverlayIcon(GetHwnd(Window), NULL, NULL);
#pragma warning( pop )
        }
    }

    void Taskbar::SetThumbnailTooltip(Gtk::Window& Window, const std::wstring& Tooltip)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetThumbnailTooltip(GetHwnd(Window), Tooltip.c_str());
        }
    }

    void Taskbar::SetThumbnailClip(Gtk::Window& Window, Gdk::Rectangle Rectangle)
    {
        if (TaskbarImpl) {
            if (Rectangle.has_zero_area()) {
#pragma warning( push )
#pragma warning( disable : 6387 )
                ((ITaskbarList3*)TaskbarImpl)->SetThumbnailClip(GetHwnd(Window), NULL);
#pragma warning( pop )
            }
            else {
                RECT Rect{
                    .left = Rectangle.get_x(),
                    .top = Rectangle.get_y(),
                    .right = Rectangle.get_x() + Rectangle.get_width(),
                    .bottom = Rectangle.get_y() + Rectangle.get_height()
                };
                ((ITaskbarList3*)TaskbarImpl)->SetThumbnailClip(GetHwnd(Window), &Rect);
            }
        }
    }
}