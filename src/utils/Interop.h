#pragma once

#include <gtkmm.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Utils::Interop {
    HWND GetHwnd(Gtk::Window& Window);

    HICON GetIcon(Gdk::Pixbuf& Pixbuf);

    HICON GetIcon(Glib::RefPtr<Gdk::Pixbuf> Pixbuf);
}