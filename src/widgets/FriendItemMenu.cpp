#include "FriendItemMenu.h"

namespace EGL3::Widgets {
    FriendItemMenu::FriendItemMenu() :
        SelectedFriend(nullptr)
    {
        Construct();
    }

    FriendItemMenu::operator Gtk::Menu&() {
        return Container;
    }

    void FriendItemMenu::PopupMenu(const Storage::Models::Friend& Friend, Gtk::Widget& TargetWidget) {
        SelectedFriend = &Friend;

        ItemChat.hide();
        ItemRemove.hide();
        ItemNickname.hide();
        ItemAccept.hide();
        ItemDecline.hide();
        ItemCancel.hide();
        ItemUnblock.hide();
        ItemBlock.show();

        switch (SelectedFriend->GetType())
        {
        case Storage::Models::FriendType::NORMAL:
            ItemChat.show();
            ItemRemove.show();
            ItemNickname.show();
            break;
        case Storage::Models::FriendType::INBOUND:
            ItemAccept.show();
            ItemDecline.show();
            break;
        case Storage::Models::FriendType::OUTBOUND:
            ItemCancel.show();
            break;
        case Storage::Models::FriendType::BLOCKED:
            ItemBlock.hide();
            ItemUnblock.show();
            break;
        }

        Container.popup_at_widget(&TargetWidget, Gdk::GRAVITY_SOUTH_WEST, Gdk::GRAVITY_NORTH_WEST, nullptr);
    }

    void FriendItemMenu::Construct() {
        ItemChat.signal_activate().connect([this]() { OnAction(ClickAction::CHAT, *SelectedFriend); });
        ItemRemove.signal_activate().connect([this]() { OnAction(ClickAction::REMOVE_FRIEND, *SelectedFriend); });
        ItemAccept.signal_activate().connect([this]() { OnAction(ClickAction::ACCEPT_REQUEST, *SelectedFriend); });
        ItemDecline.signal_activate().connect([this]() { OnAction(ClickAction::DECLINE_REQUEST, *SelectedFriend); });
        ItemCancel.signal_activate().connect([this]() { OnAction(ClickAction::CANCEL_REQUEST, *SelectedFriend); });
        ItemNickname.signal_activate().connect([this]() { OnAction(ClickAction::SET_NICKNAME, *SelectedFriend); });
        ItemBlock.signal_activate().connect([this]() { OnAction(ClickAction::BLOCK_USER, *SelectedFriend); });
        ItemUnblock.signal_activate().connect([this]() { OnAction(ClickAction::UNBLOCK_USER, *SelectedFriend); });
        ItemCopyId.signal_activate().connect([this]() { OnAction(ClickAction::COPY_USER_ID, *SelectedFriend); });

        ItemChat.set_label("Message");
        ItemRemove.set_label("Remove Friend");
        ItemNickname.set_label("Set Nickname");
        ItemAccept.set_label("Accept Request");
        ItemDecline.set_label("Decline Request");
        ItemCancel.set_label("Cancel Request");
        ItemSeparator.set_label("\xe2\xb8\xbb"); // triple em dash
        ItemBlock.set_label("Block");
        ItemUnblock.set_label("Unblock");
        ItemCopyId.set_label("Copy Account Id");

        Container.get_style_context()->add_class("font-h5");

        Container.add(ItemChat);
        Container.add(ItemRemove);
        Container.add(ItemNickname);
        Container.add(ItemAccept);
        Container.add(ItemDecline);
        Container.add(ItemCancel);
        Container.add(ItemSeparator);
        Container.add(ItemBlock);
        Container.add(ItemUnblock);
        Container.add(ItemCopyId);

        Container.show_all();
    }
}
