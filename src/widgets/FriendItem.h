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

        operator Gtk::Widget& () {
            return BaseContainer;
        }

        std::weak_ordering operator<=>(const FriendItem& that) const {
            return UpdateData <=> that.UpdateData;
        }

        void Update() {
            UpdateDispatcher.emit();
        }

        const Storage::Models::Friend& GetData() const {
            return UpdateData;
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
            auto& Friend = UpdateData.Get();

            Username.set_tooltip_text(Friend.GetAccountId());
            Username.set_text(Friend.GetDisplayName());
            Nickname.set_text(Friend.GetSecondaryName());

            AvatarBackground.set_async(Friend.GetKairosBackgroundUrl(), 64, 64, ImageCache);
            Avatar.set_async(Friend.GetKairosAvatarUrl(), 64, 64, ImageCache);

            Status.set_has_tooltip(false);

            if (UpdateData.GetType() == Storage::Models::FriendType::NORMAL || UpdateData.GetType() == Storage::Models::FriendType::CURRENT) {
                auto& FriendData = UpdateData.Get<Storage::Models::FriendType::NORMAL>();

                ColorStatus.set_async(ShowStatusToUrl(FriendData.GetShowStatus()), ImageCache);

                if (FriendData.GetShowStatus() != ShowStatus::Offline) {
                    ProductImage.set_async(GetProductImageUrl(FriendData.GetProductId()), 64, 64, ImageCache);

                    PlatformImage.set_async(GetPlatformImageUrl(FriendData.GetPlatform()), 24, 24, ImageCache);

                    if (FriendData.GetStatus().empty()) {
                        Status.set_text(ShowStatusToString(FriendData.GetShowStatus()));
                    }
                    else {
                        Status.set_text(FriendData.GetStatus());
                        Status.set_tooltip_text(FriendData.GetStatus());
                    }
                }
                else {
                    ProductImage.clear();
                    PlatformImage.clear();

                    Status.set_text(ShowStatusToString(ShowStatus::Offline));
                }
            }
            else {
                switch (UpdateData.GetType())
                {
                case Storage::Models::FriendType::INBOUND:
                    Status.set_text("Incoming Friend Request");
                    break;
                case Storage::Models::FriendType::OUTBOUND:
                    Status.set_text("Outgoing Friend Request");
                    break;
                case Storage::Models::FriendType::SUGGESTED:
                    Status.set_text("Suggested Friend");
                    break;
                case Storage::Models::FriendType::BLOCKED:
                    Status.set_text("Blocked");
                    break;
                default:
                    Status.set_text("Unknown User");
                    break;
                }

                ColorStatus.set_async(ShowStatusToUrl(ShowStatus::Offline), ImageCache);
            }
        }

        static cpr::Url GetProductImageUrl(const std::string_view ProductId) {
            if (ProductId != "EGL3") {
                std::string ProductIdLower(ProductId.data(), ProductId.size());
                std::transform(ProductIdLower.begin(), ProductIdLower.end(), ProductIdLower.begin(), [](const auto& n) { return std::tolower(n); });
                return Web::BaseClient::FormatUrl("https://cdn1.unrealengine.com/launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/%s/icon.png", ProductIdLower.c_str());
            }
            else {
                return "https://epic.gl/assets/launcher-icon.png";
            }
        }

        // https://github.com/EpicGames/UnrealEngine/blob/4da880f790851cff09ea33dadfd7aae3287878bd/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystemNames.h
        static constexpr const char* GetPlatformImageUrl(const std::string_view Platform) {
            switch (Utils::Crc32(Platform.data(), Platform.size()))
            {
            case Utils::Crc32("PSN"):
                return "https://epic.gl/assets/platforms/ps4.png";
            case Utils::Crc32("XBL"):
                return "https://epic.gl/assets/platforms/xbox.png";
            case Utils::Crc32("WIN"):
            case Utils::Crc32("MAC"):
            case Utils::Crc32("LNX"): // In the future? :)
                return "https://epic.gl/assets/platforms/pc.png";
            case Utils::Crc32("IOS"):
            case Utils::Crc32("AND"):
                return "https://epic.gl/assets/platforms/mobile.png";
            case Utils::Crc32("SWT"):
                return "https://epic.gl/assets/platforms/switch.png";
            case Utils::Crc32("OTHER"):
            default:
                return "https://epic.gl/assets/platforms/earth.png";
            }
        }

        Glib::Dispatcher UpdateDispatcher;
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
        Gtk::Label Status;
        Gtk::Overlay PlatformContainer;
        AsyncImage ProductImage;
        AsyncImage PlatformImage;
    };
}
