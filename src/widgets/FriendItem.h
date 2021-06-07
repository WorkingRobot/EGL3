#pragma once

#include "../modules/ImageCache.h"
#include "AsyncImage.h"
#include "FriendItemMenu.h"
#include "KairosAvatar.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class FriendItem {
    public:
        FriendItem(Modules::ImageCacheModule& ImageCache);
        FriendItem(const Storage::Models::Friend& Item, Modules::ImageCacheModule& ImageCache);

        FriendItem(FriendItem&&) = default;
        FriendItem& operator=(FriendItem&&) = default;

        operator Gtk::Widget& ();

        std::weak_ordering operator<=>(const FriendItem& that) const;

        void SetContextMenu(Widgets::FriendItemMenu& ContextMenu);

        void Update();

        const Storage::Models::Friend& GetData() const;

        void SetData(const Storage::Models::Friend& NewItem);

    private:
        void SetContextMenuInternal();

        void Construct();

        void UpdateDispatch();

        Glib::Dispatcher UpdateDispatcher;
        const Storage::Models::Friend* UpdateData;

    protected:
        void OnMapConstruct();

        struct FriendItemInternal {
            Gtk::Box BaseBox{ Gtk::ORIENTATION_HORIZONTAL };
            Gtk::Overlay AvatarContainer;
            KairosAvatar Avatar;
            Gtk::EventBox AvatarEventBox;
            Gtk::EventBox ColorStatusEventBox;
            AsyncImage ColorStatus;
            Gtk::Box StatusContainer{ Gtk::ORIENTATION_VERTICAL };
            Gtk::Box UsernameContainer{ Gtk::ORIENTATION_HORIZONTAL };
            Gtk::Label Username;
            Gtk::Label Nickname;
            Gtk::Label Status;
            Gtk::Overlay PlatformContainer;
            AsyncImage ProductImage;
            AsyncImage PlatformImage;
            Modules::ImageCacheModule& ImageCache;

            FriendItemInternal(Modules::ImageCacheModule& ImageCache);

            void Construct(Gtk::EventBox& BaseContainer);

            void Update(const Storage::Models::Friend* UpdateData);
        };

        Modules::ImageCacheModule& ImageCache;
        Gtk::EventBox BaseContainer;
        Widgets::FriendItemMenu* ContextMenu;
        sigc::connection ContextMenuConnection;
        std::unique_ptr<FriendItemInternal> Data;
    };
}
