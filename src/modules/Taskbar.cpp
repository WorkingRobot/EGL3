#include "Taskbar.h"

namespace EGL3::Modules {
    TaskbarModule::TaskbarModule(const Utils::GladeBuilder& Builder) :
        MainWindow(Builder.GetWidget<Gtk::Window>("EGL3App"))
    {

    }

    void TaskbarModule::SetProgressValue(uint64_t Value, uint64_t Total)
    {
        TaskbarImpl.SetProgressValue(MainWindow, Value, Total);
    }

    void TaskbarModule::SetProgressState(Utils::Taskbar::ProgressState NewState)
    {
        TaskbarImpl.SetProgressState(MainWindow, NewState);
    }

    void TaskbarModule::AddThumbbarButtons(const std::vector<Utils::Taskbar::ThumbButton>& Buttons)
    {
        TaskbarImpl.AddThumbbarButtons(MainWindow, Buttons);
    }

    void TaskbarModule::UpdateThumbbarButtons(const std::vector<Utils::Taskbar::ThumbButton>& Buttons)
    {
        TaskbarImpl.UpdateThumbbarButtons(MainWindow, Buttons);
    }

    void TaskbarModule::SetOverlayIcon(Gdk::Pixbuf& Icon)
    {
        TaskbarImpl.SetOverlayIcon(MainWindow, Icon);
    }

    void TaskbarModule::SetOverlayIcon(Gdk::Pixbuf& Icon, const std::wstring& Description)
    {
        TaskbarImpl.SetOverlayIcon(MainWindow, Icon, Description);
    }

    void TaskbarModule::RemoveOverlayIcon()
    {
        TaskbarImpl.RemoveOverlayIcon(MainWindow);
    }

    void TaskbarModule::SetThumbnailTooltip(const std::wstring& Tooltip)
    {
        TaskbarImpl.SetThumbnailTooltip(MainWindow, Tooltip);
    }

    void TaskbarModule::SetThumbnailClip(Gdk::Rectangle Rectangle)
    {
        TaskbarImpl.SetThumbnailClip(MainWindow, Rectangle);
    }
}