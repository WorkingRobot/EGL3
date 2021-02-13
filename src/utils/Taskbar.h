#pragma once

#include <gtkmm.h>

#include <vector>

namespace EGL3::Utils {
    class Taskbar {
    public:
        enum class ProgressState : uint8_t {
            None            = 0x0,
            Indeterminate   = 0x1,
            Normal          = 0x2,
            Error           = 0x4,
            Paused          = 0x8
        };

        enum class ThumbButtonFlags : uint8_t {
            Enabled         = 0x0,
            Disabled        = 0x1,
            DismissOnClick  = 0x2,
            NoBackground    = 0x4,
            Hidden          = 0x8,
            NonInteractive  = 0x10,

            // Set this bit if you want the flags to apply
            // Otherwise, the flags field will be ignored
            InUse           = 0x20,
        };

        struct ThumbButton {
            uint32_t Id;
            Gdk::Pixbuf* Icon;
            std::wstring Tooltip;
            ThumbButtonFlags Flags;
        };

        Taskbar();

        ~Taskbar();

        void SetProgressValue(Gtk::Window& Window, uint64_t Value, uint64_t Total);

        void SetProgressState(Gtk::Window& Window, ProgressState NewState);

        void AddThumbbarButtons(Gtk::Window& Window, const std::vector<ThumbButton>& Buttons);

        void UpdateThumbbarButtons(Gtk::Window& Window, const std::vector<ThumbButton>& Buttons);

        void SetOverlayIcon(Gtk::Window& Window, Gdk::Pixbuf& Icon);

        void SetOverlayIcon(Gtk::Window& Window, Gdk::Pixbuf& Icon, const std::wstring& Description);

        void RemoveOverlayIcon(Gtk::Window& Window);

        void SetThumbnailTooltip(Gtk::Window& Window, const std::wstring& Tooltip);

        void SetThumbnailClip(Gtk::Window& Window, Gdk::Rectangle Rectangle);

    private:
        bool CoInitialized;
        void* TaskbarImpl;
    };
}