#include "Taskbar.h"

#include "Interop.h"
#include "Log.h"

#include <ShObjIdl.h>

namespace EGL3::Utils {
    Taskbar::Taskbar() :
        TaskbarImpl(nullptr)
    {
        HRESULT Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        EGL3_VERIFY(Result != CO_E_NOTINITIALIZED, "COM couldn't be initialized");
        CoInitialized = true;
        EGL3_ENSURE(Result != RPC_E_CHANGED_MODE, LogLevel::Warning, "COM has changed mode, this may be problematic");

        Result = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (LPVOID*)&TaskbarImpl);
        EGL3_VERIFY(Result == S_OK, "Could not get taskbar COM interface");
    }

    Taskbar::~Taskbar() {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->Release();
        }
        CoUninitialize();
    }

    void Taskbar::SetProgressValue(Gtk::Window& Window, uint64_t Value, uint64_t Total)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetProgressValue(Interop::GetHwnd(Window), Value, Total);
        }
    }

    void Taskbar::SetProgressState(Gtk::Window& Window, ProgressState NewState)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetProgressState(Interop::GetHwnd(Window), (TBPFLAG)NewState);
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
                ButtonImpl.hIcon = Interop::GetIcon(*Button.Icon);
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

            ((ITaskbarList3*)TaskbarImpl)->ThumbBarAddButtons(Interop::GetHwnd(Window), (uint32_t)Buttons.size(), ButtonsImpl.get());
        }
    }

    void Taskbar::UpdateThumbbarButtons(Gtk::Window& Window, const std::vector<ThumbButton>& Buttons)
    {
        if (TaskbarImpl) {
            auto ButtonsImpl = ConvertButtons(Buttons);

            ((ITaskbarList3*)TaskbarImpl)->ThumbBarUpdateButtons(Interop::GetHwnd(Window), (uint32_t)Buttons.size(), ButtonsImpl.get());
        }
    }

    void Taskbar::SetOverlayIcon(Gtk::Window& Window, Gdk::Pixbuf& Icon)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetOverlayIcon(Interop::GetHwnd(Window), Interop::GetIcon(Icon), NULL);
        }
    }

    void Taskbar::SetOverlayIcon(Gtk::Window& Window, Gdk::Pixbuf& Icon, const std::wstring& Description)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetOverlayIcon(Interop::GetHwnd(Window), Interop::GetIcon(Icon), Description.c_str());
        }
    }

    void Taskbar::RemoveOverlayIcon(Gtk::Window& Window)
    {
        if (TaskbarImpl) {
#pragma warning( push )
#pragma warning( disable : 6387 )
            ((ITaskbarList3*)TaskbarImpl)->SetOverlayIcon(Interop::GetHwnd(Window), NULL, NULL);
#pragma warning( pop )
        }
    }

    void Taskbar::SetThumbnailTooltip(Gtk::Window& Window, const std::wstring& Tooltip)
    {
        if (TaskbarImpl) {
            ((ITaskbarList3*)TaskbarImpl)->SetThumbnailTooltip(Interop::GetHwnd(Window), Tooltip.c_str());
        }
    }

    void Taskbar::SetThumbnailClip(Gtk::Window& Window, Gdk::Rectangle Rectangle)
    {
        if (TaskbarImpl) {
            if (Rectangle.has_zero_area()) {
#pragma warning( push )
#pragma warning( disable : 6387 )
                ((ITaskbarList3*)TaskbarImpl)->SetThumbnailClip(Interop::GetHwnd(Window), NULL);
#pragma warning( pop )
            }
            else {
                RECT Rect{
                    .left = Rectangle.get_x(),
                    .top = Rectangle.get_y(),
                    .right = Rectangle.get_x() + Rectangle.get_width(),
                    .bottom = Rectangle.get_y() + Rectangle.get_height()
                };
                ((ITaskbarList3*)TaskbarImpl)->SetThumbnailClip(Interop::GetHwnd(Window), &Rect);
            }
        }
    }
}