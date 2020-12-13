#pragma once

#include <gtkmm.h>

#include "AsyncImage.h"
#include "../modules/ImageCache.h"

namespace EGL3::Widgets {
    using namespace Web::Xmpp::Json;

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

        void Update() {
            UpdateDispatcher.emit();
        }

        operator Gtk::Widget& () {
            return BaseContainer;
        }

        std::weak_ordering operator<=>(const FriendItem& that) const {
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

            Status.set_line_wrap(true);
            Status.set_ellipsize(Pango::ELLIPSIZE_END);
            Status.set_lines(1);

            Username.get_style_context()->add_class("font-bold");
            Username.get_style_context()->add_class("font-h5");
            Nickname.get_style_context()->add_class("font-translucent");
            Nickname.get_style_context()->add_class("font-h5");
            Status.get_style_context()->add_class("font-h5");

            Favorited.set_valign(Gtk::ALIGN_END);

            ColorStatusEventBox.add(ColorStatus);

            ColorStatusEventBox.set_halign(Gtk::ALIGN_END);
            ColorStatusEventBox.set_valign(Gtk::ALIGN_END);

            AvatarEventBox.add(Avatar);

            AvatarContainer.set_size_request(72, 72);

            AvatarContainer.add(AvatarBackground);
            AvatarContainer.add_overlay(AvatarEventBox);
            AvatarContainer.add_overlay(ColorStatusEventBox);

            AvatarContainer.set_overlay_pass_through(AvatarEventBox, false);
            AvatarContainer.set_overlay_pass_through(ColorStatusEventBox, false);

            UsernameContainer.pack_start(Username, false, false, 0);
            UsernameContainer.pack_start(Nickname, false, false, 0);
            UsernameContainer.pack_end(Favorited, false, false, 0);

            StatusContainer.pack_start(UsernameContainer, true, true, 5);
            StatusContainer.pack_end(Status, true, true, 5);

            PlatformImage.set_halign(Gtk::ALIGN_END);
            PlatformImage.set_valign(Gtk::ALIGN_END);

            PlatformContainer.set_size_request(72, 72);

            PlatformContainer.add(ProductImage);
            PlatformContainer.add_overlay(PlatformImage);

            BaseContainer.pack_start(AvatarContainer, false, false, 0);
            BaseContainer.pack_start(StatusContainer, true, true, 5);
            BaseContainer.pack_end(PlatformContainer, false, false, 0);

            BaseContainer.show_all();
        }

        void UpdateDispatch() {
            std::lock_guard UpdateDataLock(UpdateDataMutex);

            Username.set_tooltip_text(UpdateData.GetAccountId());
            Username.set_text(UpdateData.GetDisplayName());
            Nickname.set_text(UpdateData.GetAlternateName());
            // set_visible or show/hide doesn't work for some reason
            if (UpdateData.IsFavorited()) {
                Favorited.set_from_icon_name("starred-symbolic", Gtk::ICON_SIZE_BUTTON);
            }
            else {
                Favorited.clear();
            }

            AvatarBackground.set_async(UpdateData.GetKairosBackgroundUrl(), 64, 64, ImageCache);
            Avatar.set_async(UpdateData.GetKairosAvatarUrl(), 64, 64, ImageCache);

            ColorStatus.set_async(ShowStatusToUrl(UpdateData.GetShowStatus()), ImageCache);

            if (UpdateData.GetShowStatus() != ShowStatus::Offline) {
                auto ProductId = UpdateData.GetProductId();
                switch (Utils::Crc32(ProductId.data(), ProductId.size()))
                {
                case Utils::Crc32("EGL3"):
                    ProductImage.set_async("https://fnbot.shop/egl3/launcher-icon.png", 64, 64, ImageCache);
                    break;
                case Utils::Crc32("Fortnite"):
                    ProductImage.set_async("http://cdn1.unrealengine.com/launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/fortnite/icon.png", 64, 64, ImageCache);
                    break;
                default:
                    ProductImage.set_async("http://cdn1.unrealengine.com/launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/" + std::string(ProductId) + "/icon.png", 64, 64, ImageCache);
                    break;
                }

                // https://github.com/EpicGames/UnrealEngine/blob/4da880f790851cff09ea33dadfd7aae3287878bd/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystemNames.h
                auto Platform = UpdateData.GetPlatform();
                switch (Utils::Crc32(Platform.data(), Platform.size()))
                {
                case Utils::Crc32("PSN"):
                    PlatformImage.set_async("https://fnbot.shop/egl3/platforms/ps4.png", 24, 24, ImageCache);
                    break;
                case Utils::Crc32("XBL"):
                    PlatformImage.set_async("https://fnbot.shop/egl3/platforms/xbox.png", 24, 24, ImageCache);
                    break;
                case Utils::Crc32("WIN"):
                case Utils::Crc32("MAC"):
                case Utils::Crc32("LNX"): // In the future? :)
                    PlatformImage.set_async("https://fnbot.shop/egl3/platforms/pc.png", 24, 24, ImageCache);
                    break;
                case Utils::Crc32("IOS"):
                case Utils::Crc32("AND"):
                    PlatformImage.set_async("https://fnbot.shop/egl3/platforms/mobile.png", 24, 24, ImageCache);
                    break;
                case Utils::Crc32("SWT"):
                    PlatformImage.set_async("https://fnbot.shop/egl3/platforms/switch.png", 24, 24, ImageCache);
                    break;
                case Utils::Crc32("OTHER"):
                default:
                    PlatformImage.set_async("https://fnbot.shop/egl3/platforms/earth.png", 24, 24, ImageCache);
                    break;
                }

                Status.set_text(UpdateData.GetStatus().empty() ? ShowStatusToString(UpdateData.GetShowStatus()) : UpdateData.GetStatus());
            }
            else {
                ProductImage.clear();
                PlatformImage.clear();

                if (UpdateData.GetRelationStatus() != Storage::Models::Friend::RelationStatus::ACCEPTED) {
                    switch (UpdateData.GetRelationStatus()) {
                    case Storage::Models::Friend::RelationStatus::PENDING:
                        switch (UpdateData.GetRelationDirection())
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
                        Status.set_text("Suggested Friend");
                        break;
                    default:
                        Status.set_text("Unknown Friend");
                        break;
                    }
                }
                else {
                    Status.set_text(ShowStatusToString(ShowStatus::Offline));
                }
            }
        }

        Glib::Dispatcher UpdateDispatcher;
        // Mutable because operator<=> locks it
        mutable std::mutex UpdateDataMutex;
        const Storage::Models::Friend& UpdateData;

    protected:
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
        Gtk::Overlay PlatformContainer;
        AsyncImage ProductImage;
        AsyncImage PlatformImage;
    };
}
