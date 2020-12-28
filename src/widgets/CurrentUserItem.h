#pragma once

#include "FriendItem.h"

namespace EGL3::Widgets {
    class CurrentUserItem : public FriendItem {
    public:
        CurrentUserItem(const Storage::Models::Friend& Item, Modules::ImageCacheModule& ImageCache);

        void SetAsCurrentUser(Gtk::Window& KairosMenu);

    protected:
        void DisplayMenu(Gtk::Window& KairosMenu, GdkEventButton* evt);
    };
}
