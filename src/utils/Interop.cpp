#include "Interop.h"

#include "Log.h"

// This #define is for gdk_win32_pixbuf_to_hicon_libgtk_only
// Sorry for the hacky solutions, but they're too useful to just not use
#define GDK_COMPILATION
#include <gdk/gdkwin32.h>

namespace EGL3::Utils::Interop {
    HWND GetHwnd(Gtk::Window& Window) {
        GdkWindow* InternalWindow = Window.get_window()->gobj();
        EGL3_VERIFY(gdk_win32_window_is_win32(InternalWindow), "Window is not a Win32 window");

        return gdk_win32_window_get_impl_hwnd(InternalWindow);
    }

    HICON GetIcon(Gdk::Pixbuf& Pixbuf) {
        return gdk_win32_pixbuf_to_hicon_libgtk_only(Pixbuf.gobj());
    }

    HICON GetIcon(Glib::RefPtr<Gdk::Pixbuf> Pixbuf)
    {
        return gdk_win32_pixbuf_to_hicon_libgtk_only(Pixbuf->gobj());
    }
}