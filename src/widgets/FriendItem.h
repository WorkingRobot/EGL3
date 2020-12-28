#pragma once

#include "../modules/ImageCache.h"
#include "AsyncImage.h"
#include "FriendItemMenu.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class FriendItem {
    public:
        FriendItem(const Storage::Models::Friend& Item, Modules::ImageCacheModule& ImageCache);

        FriendItem(FriendItem&&) = default;
        FriendItem& operator=(FriendItem&&) = default;

        operator Gtk::Widget& ();

        std::weak_ordering operator<=>(const FriendItem& that) const;

        void SetContextMenu(Widgets::FriendItemMenu& ContextMenu);

        void Update();

        const Storage::Models::Friend& GetData() const;

    private:
        void Construct();

        void UpdateDispatch();

        static std::string GetProductImageUrl(std::string_view ProductId);

        // https://github.com/EpicGames/UnrealEngine/blob/4da880f790851cff09ea33dadfd7aae3287878bd/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystemNames.h
        static constexpr const char* GetPlatformImageUrl(const std::string_view Platform);

        Glib::Dispatcher UpdateDispatcher;
        const Storage::Models::Friend& UpdateData;

    protected:
        Modules::ImageCacheModule& ImageCache;
        Gtk::EventBox BaseContainer;
        Gtk::Box BaseBox{ Gtk::ORIENTATION_HORIZONTAL };
        Gtk::Overlay AvatarContainer;
        AsyncImage AvatarBackground;
        Gtk::EventBox AvatarEventBox;
        AsyncImage Avatar;
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
    };
}
