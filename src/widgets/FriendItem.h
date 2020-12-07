#pragma once

#include <gtkmm.h>

#include "AsyncImage.h"
#include "../modules/ImageCache.h"

namespace EGL3::Widgets {
    class FriendItem {
    public:
        FriendItem(const Storage::Models::Friend& Item, Modules::ImageCacheModule& ImageCache) :
            UpdateData(Item),
            ImageCache(ImageCache)
        {
            UpdateDispatcher.connect([this]() { UpdateDispatch(); });

            Construct();

            UpdateDispatch();
        }

        FriendItem(FriendItem&&) = default;
        FriendItem& operator=(FriendItem&&) = default;

        void SetAsCurrentUser(Gtk::Window& KairosMenu) {
            AvatarEventBox.signal_button_press_event().connect([&, this](GdkEventButton* evt) {
                KairosMenu.set_attached_to(Avatar);
                KairosMenu.show();
                KairosMenu.present();

                auto Alloc = Avatar.get_allocation();
                int x, y;
                Avatar.get_window()->get_origin(x, y);
                // Moved after because get_height returns 1 when hidden (couldn't find other workarounds for that, and I'm not bothered since there's no flashing)
                KairosMenu.move(x + Alloc.get_x(), y + Alloc.get_y() - KairosMenu.get_height());
                return true;
            });
        }

        void Update() {
            UpdateDispatcher.emit();
        }

        operator Gtk::Widget& () {
            return BaseContainer;
        }

        std::strong_ordering operator<=>(const FriendItem& that) const {
            std::scoped_lock DoubleLock(UpdateDataMutex, that.UpdateDataMutex);
            return UpdateData <=> that.UpdateData;
        }

    private:
        void Construct() {
            BaseContainer.set_data("EGL3_FriendBase", this);

            BaseContainer.set_border_width(2);

            Username.set_xalign(0);
            Username.set_yalign(0);

            Nickname.set_xalign(0);
            Nickname.set_yalign(0);

            Status.set_xalign(0);
            Status.set_yalign(0);

            Nickname.set_margin_left(10);

            Status.set_line_wrap(true);
            Status.set_ellipsize(Pango::ELLIPSIZE_END);
            Status.set_lines(1);

            Username.get_style_context()->add_class("font-bold");
            Username.get_style_context()->add_class("font-h5");
            Nickname.get_style_context()->add_class("font-translucent");
            Nickname.get_style_context()->add_class("font-h5");
            Status.get_style_context()->add_class("font-h5");

            Favorited.set_valign(Gtk::ALIGN_END);

            AvatarBackground.set_async(UpdateData.Kairos.GetBackgroundUrl(), 64, 64, ImageCache);
            Avatar.set_async(UpdateData.Kairos.GetAvatarUrl(), 64, 64, ImageCache);

            ColorStatusEventBox.add(ColorStatus);
            ColorStatusEventBox.set_events(Gdk::BUTTON_PRESS_MASK);

            ColorStatusEventBox.set_halign(Gtk::ALIGN_END);
            ColorStatusEventBox.set_valign(Gtk::ALIGN_END);

            AvatarEventBox.add(Avatar);
            AvatarEventBox.set_events(Gdk::BUTTON_PRESS_MASK);

            AvatarContainer.set_size_request(72, 72);

            AvatarContainer.add(AvatarBackground);
            AvatarContainer.add_overlay(AvatarEventBox);
            AvatarContainer.add_overlay(ColorStatusEventBox);

            AvatarContainer.set_overlay_pass_through(AvatarEventBox, false);
            AvatarContainer.set_overlay_pass_through(ColorStatusEventBox, false);

            UsernameContainer.pack_start(Username, false, false, 0);
            UsernameContainer.pack_start(Nickname, false, false, 10);
            UsernameContainer.pack_end(Favorited, false, false, 0);

            StatusContainer.pack_start(UsernameContainer, true, true, 5);
            StatusContainer.pack_end(Status, true, true, 5);

            BaseContainer.pack_start(AvatarContainer, false, false, 0);
            BaseContainer.pack_start(StatusContainer, true, true, 5);
            BaseContainer.pack_end(PlayImage, false, false, 0);

            BaseContainer.show_all();
        }

        void UpdateDispatch() {
            std::lock_guard UpdateDataLock(UpdateDataMutex);

            Username.set_tooltip_text(UpdateData.AccountId);
            Username.set_text(UpdateData.Username);
            Nickname.set_text(UpdateData.Nickname);
            // set_visible or show/hide doesn't work for some reason
            if (UpdateData.Favorite) {
                Favorited.set_from_icon_name("starred-symbolic", Gtk::ICON_SIZE_BUTTON);
            }
            else {
                Favorited.clear();
            }

            Avatar.set_async(UpdateData.Kairos.GetAvatarUrl(), 64, 64, ImageCache);
            AvatarBackground.set_async(UpdateData.Kairos.GetBackgroundUrl(), 64, 64, ImageCache);

            switch (UpdateData.OnlineStatus)
            {
            case Web::Xmpp::Json::ShowStatus::Online:
                ColorStatus.set_async("https://fnbot.shop/egl3/status/online.png", ImageCache);
                break;
            case Web::Xmpp::Json::ShowStatus::Chat:
                ColorStatus.set_async("https://fnbot.shop/egl3/status/chat.png", ImageCache);
                break;
            case Web::Xmpp::Json::ShowStatus::DoNotDisturb:
                ColorStatus.set_async("https://fnbot.shop/egl3/status/dnd.png", ImageCache);
                break;
            case Web::Xmpp::Json::ShowStatus::Away:
                ColorStatus.set_async("https://fnbot.shop/egl3/status/away.png", ImageCache);
                break;
            case Web::Xmpp::Json::ShowStatus::ExtendedAway:
                ColorStatus.set_async("https://fnbot.shop/egl3/status/xa.png", ImageCache);
                break;
            case Web::Xmpp::Json::ShowStatus::Offline:
                ColorStatus.set_async("https://fnbot.shop/egl3/status/offline.png", ImageCache);
                break;
            }

            if (UpdateData.OnlineStatus != Web::Xmpp::Json::ShowStatus::Offline) {
                switch (Utils::Crc32(UpdateData.ProductId))
                {
                case Utils::Crc32("EGL3"):
                    PlayImage.set_async("https://fnbot.shop/egl3/status/launcher-icon.png", 64, 64, ImageCache);
                    break;
                case Utils::Crc32("Fortnite"):
                    PlayImage.set_async("http://cdn1.unrealengine.com/launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/fortnite/icon.png", 64, 64, ImageCache);
                    break;
                default:
                    PlayImage.set_async("http://cdn1.unrealengine.com/launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/launcher/icon.png", 64, 64, ImageCache);
                    break;
                }

                Status.set_text(UpdateData.DisplayStatus.empty() ? "In the launcher" : UpdateData.DisplayStatus);
            }
            else {
                PlayImage.clear();

                if (UpdateData.Status != Storage::Models::Friend::RelationStatus::ACCEPTED) {
                    switch (UpdateData.Status) {
                    case Storage::Models::Friend::RelationStatus::PENDING:
                        switch (UpdateData.Direction)
                        {
                        case Storage::Models::Friend::RelationDirection::INBOUND:
                            Status.set_text("Incoming Friend Request");
                            break;
                        case Storage::Models::Friend::RelationDirection::OUTBOUND:
                            Status.set_text("Outgoing Friend Request");
                            break;
                        default:
                            Status.set_text("Pending Friend Request");
                            break;
                        }
                        break;
                    case Storage::Models::Friend::RelationStatus::SUGGESTED:
                        Status.set_text("Suggested");
                        break;
                    default:
                        Status.set_text("Unknown Friend");
                        break;
                    }
                }
                else {
                    Status.set_text("Offline");
                }
            }
        }

        Glib::Dispatcher UpdateDispatcher;
        // Mutable because operator<=> locks it
        mutable std::mutex UpdateDataMutex;
        const Storage::Models::Friend& UpdateData;

        Modules::ImageCacheModule& ImageCache;
        Gtk::Box BaseContainer{ Gtk::ORIENTATION_HORIZONTAL };
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
        Gtk::Image Favorited;
        Gtk::Label Status;
        AsyncImage PlayImage;
    };
}
