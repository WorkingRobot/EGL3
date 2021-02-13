#pragma once

#include "BaseModule.h"

#include "../utils/GladeBuilder.h"
#include "../utils/Taskbar.h"

namespace EGL3::Modules {
    class TaskbarModule : public BaseModule {
        Gtk::Window& MainWindow;
        Utils::Taskbar TaskbarImpl;

    public:
        TaskbarModule(const Utils::GladeBuilder& Builder);

        void SetProgressValue(uint64_t Value, uint64_t Total);

        void SetProgressState(Utils::Taskbar::ProgressState NewState);

        void AddThumbbarButtons(const std::vector<Utils::Taskbar::ThumbButton>& Buttons);

        void UpdateThumbbarButtons(const std::vector<Utils::Taskbar::ThumbButton>& Buttons);

        void SetOverlayIcon(Gdk::Pixbuf& Icon);

        void SetOverlayIcon(Gdk::Pixbuf& Icon, const std::wstring& Description);

        void RemoveOverlayIcon();

        void SetThumbnailTooltip(const std::wstring& Tooltip);

        void SetThumbnailClip(Gdk::Rectangle Rectangle);
    };
}