#pragma once

#include "FriendItem.h"

namespace EGL3::Widgets {
    class CurrentUserItem : public FriendItem {
    public:
        CurrentUserItem(const Storage::Models::Friend& Item, Modules::ImageCacheModule& ImageCache) : FriendItem(Item, ImageCache) {}

        void SetAsCurrentUser(Gtk::Window& KairosMenu) {
            AvatarEventBox.set_events(Gdk::BUTTON_RELEASE_MASK);
            ColorStatusEventBox.set_events(Gdk::BUTTON_RELEASE_MASK);
            AvatarEventBox.signal_button_release_event().connect([&, this](GdkEventButton* evt) { DisplayMenu(KairosMenu, evt); return true; });
            ColorStatusEventBox.signal_button_release_event().connect([&, this](GdkEventButton* evt) { DisplayMenu(KairosMenu, evt); return true; });
        }

    protected:
        void DisplayMenu(Gtk::Window& KairosMenu, GdkEventButton* evt) {
            KairosMenu.set_attached_to(Avatar);
            KairosMenu.show();
            KairosMenu.present();

            auto Alloc = Avatar.get_allocation();
            int x, y;
            Avatar.get_window()->get_origin(x, y);
            // Moved after because get_height returns 1 when hidden (couldn't find other workarounds for that, and I'm not bothered since there's no flickering)
            KairosMenu.move(x + Alloc.get_x(), y + Alloc.get_y() - KairosMenu.get_height());
        }
    };
}
