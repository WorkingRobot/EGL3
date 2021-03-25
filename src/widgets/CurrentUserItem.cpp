#include "CurrentUserItem.h"

namespace EGL3::Widgets {
    CurrentUserItem::CurrentUserItem(const Storage::Models::Friend& Item, Modules::ImageCacheModule& ImageCache) :
        FriendItem(Item, ImageCache)
    {

    }

    void CurrentUserItem::SetAsCurrentUser(Gtk::Window& KairosMenu) {
        OnMapConstruct();

        Data->AvatarEventBox.set_events(Gdk::BUTTON_RELEASE_MASK);
        Data->ColorStatusEventBox.set_events(Gdk::BUTTON_RELEASE_MASK);
        Data->AvatarEventBox.signal_button_release_event().connect([&, this](GdkEventButton* evt) { DisplayMenu(KairosMenu); return true; });
        Data->ColorStatusEventBox.signal_button_release_event().connect([&, this](GdkEventButton* evt) { DisplayMenu(KairosMenu); return true; });
    }

    void CurrentUserItem::DisplayMenu(Gtk::Window& KairosMenu) {
        KairosMenu.set_attached_to(Data->Avatar);
        KairosMenu.show();
        KairosMenu.present();

        auto Alloc = Data->Avatar.get_allocation();
        int x, y;
        Data->Avatar.get_window()->get_origin(x, y);
        // Moved after because get_height returns 1 when hidden (couldn't find other workarounds for that, and I'm not bothered since there's no flickering)
        KairosMenu.move(x + Alloc.get_x(), y + Alloc.get_y() - KairosMenu.get_height());
    }
}
