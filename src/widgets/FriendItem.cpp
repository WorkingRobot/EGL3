#include "FriendItem.h"

#include "../utils/Crc32.h"
#include "../utils/Format.h"
#include "../web/Hosts.h"

namespace EGL3::Widgets {
    using namespace Web::Xmpp::Json;

    FriendItem::FriendItem(const Storage::Models::Friend& Item, Modules::ImageCacheModule& ImageCache) :
        UpdateData(&Item),
        ImageCache(ImageCache)
    {
        UpdateDispatcher.connect([this]() { UpdateDispatch(); });

        Construct();

        UpdateDispatch();
    }

    FriendItem::FriendItem(Modules::ImageCacheModule& ImageCache) :
        UpdateData(nullptr),
        ImageCache(ImageCache)
    {
        UpdateDispatcher.connect([this]() { UpdateDispatch(); });

        Construct();
    }

    FriendItem::operator Gtk::Widget& () {
        return BaseContainer;
    }

    std::weak_ordering FriendItem::operator<=>(const FriendItem& that) const {
        return GetData() <=> that.GetData();
    }

    void FriendItem::SetContextMenu(Widgets::FriendItemMenu& ContextMenu) {
        BaseContainer.set_events(Gdk::BUTTON_RELEASE_MASK);
        BaseContainer.signal_button_release_event().connect([&, this](GdkEventButton* evt) { ContextMenu.PopupMenu(GetData(), BaseBox); return true; });
    }

    void FriendItem::Update() {
        UpdateDispatcher.emit();
    }

    const Storage::Models::Friend& FriendItem::GetData() const {
        return *UpdateData;
    }

    void FriendItem::SetData(const Storage::Models::Friend& NewItem)
    {
        UpdateData = &NewItem;
    }

    void FriendItem::Construct() {
        BaseContainer.set_data("EGL3_FriendBase", this);

        BaseBox.set_border_width(2);

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

        BaseBox.pack_start(AvatarContainer, false, false, 0);
        BaseBox.pack_start(StatusContainer, true, true, 5);
        BaseBox.pack_end(PlatformContainer, false, false, 0);

        BaseContainer.add(BaseBox);
        BaseContainer.show_all();
    }

    void FriendItem::UpdateDispatch() {
        if (!UpdateData) {
            return;
        }

        auto& Friend = UpdateData->Get();

        Username.set_tooltip_text(Friend.GetAccountId());
        Username.set_text(Friend.GetDisplayName());
        Nickname.set_text(Friend.GetSecondaryName());

        AvatarBackground.set_async(Friend.GetKairosBackgroundUrl(), PresenceKairosProfile::GetDefaultKairosBackgroundUrl(), 64, 64, ImageCache);
        Avatar.set_async(Friend.GetKairosAvatarUrl(), PresenceKairosProfile::GetDefaultKairosAvatarUrl(), 64, 64, ImageCache);

        Status.set_has_tooltip(false);

        if (UpdateData->GetType() == Storage::Models::FriendType::NORMAL || UpdateData->GetType() == Storage::Models::FriendType::CURRENT) {
            auto& FriendData = UpdateData->Get<Storage::Models::FriendReal>();

            ColorStatus.set_async(ShowStatusToUrl(FriendData.GetShowStatus()), "", ImageCache);

            if (FriendData.GetShowStatus() != ShowStatus::Offline) {
                ProductImage.set_async(GetProductImageUrl(FriendData.GetProductId()), GetProductImageUrl("default"), 64, 64, ImageCache);

                PlatformImage.set_async(GetPlatformImageUrl(FriendData.GetPlatform()), GetPlatformImageUrl("OTHER"), 24, 24, ImageCache);

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
            switch (UpdateData->GetType())
            {
            case Storage::Models::FriendType::INBOUND:
                Status.set_text("Incoming Friend Request");
                break;
            case Storage::Models::FriendType::OUTBOUND:
                Status.set_text("Outgoing Friend Request");
                break;
            case Storage::Models::FriendType::BLOCKED:
                Status.set_text("Blocked");
                break;
            default:
                Status.set_text("Unknown User");
                break;
            }

            ColorStatus.set_async(ShowStatusToUrl(ShowStatus::Offline), "", ImageCache);
        }
    }

    std::string FriendItem::GetProductImageUrl(std::string_view ProductId) {
        switch (Utils::Crc32(ProductId.data(), ProductId.size())) {
        case Utils::Crc32("EGL3"):
            return Utils::Format("%slauncher-icon.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("Fortnite"):
            ProductId = "fortnite";
        default:
            return Utils::Format("%slauncher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/%.*s/icon.png", Web::GetHostUrl<Web::Host::UnrealEngineCdn1>(), ProductId.size(), ProductId.data());
        }
    }

    // https://github.com/EpicGames/UnrealEngine/blob/4da880f790851cff09ea33dadfd7aae3287878bd/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystemNames.h
    std::string FriendItem::GetPlatformImageUrl(const std::string_view Platform) {
        switch (Utils::Crc32(Platform.data(), Platform.size()))
        {
        case Utils::Crc32("PSN"):
        case Utils::Crc32("PS5"):
            return Utils::Format("%splatforms/ps4.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("XBL"):
            return Utils::Format("%splatforms/xbox.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("WIN"):
        case Utils::Crc32("MAC"):
        case Utils::Crc32("LNX"): // In the future? :)
            return Utils::Format("%splatforms/pc.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("IOS"):
        case Utils::Crc32("AND"):
            return Utils::Format("%splatforms/mobile.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("SWT"):
            return Utils::Format("%splatforms/switch.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("OTHER"):
        default:
            return Utils::Format("%splatforms/earth.png", Web::GetHostUrl<Web::Host::EGL3>());
        }
    }
}
