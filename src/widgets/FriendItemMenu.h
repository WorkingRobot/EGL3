#pragma once

#include <gtkmm.h>

#include "../storage/models/Friend.h"

namespace EGL3::Widgets {
    class FriendItemMenu {
    public:
        enum class ClickAction : uint8_t {
            CHAT,
            REMOVE_FRIEND,
            SET_NICKNAME,
            ACCEPT_REQUEST,
            DECLINE_REQUEST,
            CANCEL_REQUEST,
            SEND_REQUEST,
            BLOCK_USER,
            UNBLOCK_USER,
            COPY_USER_ID
        };

        FriendItemMenu(const std::function<void(ClickAction)>& OnAction) :
            OnAction(OnAction)
        {
            Construct();
        }

        operator Gtk::Menu&() {
            return Container;
        }

        void PopupMenu(const Storage::Models::Friend& Data, Gtk::Widget& TargetWidget) {
            ItemChat.hide();
            ItemRemove.hide();
            ItemNickname.hide();
            ItemAccept.hide();
            ItemDecline.hide();
            ItemCancel.hide();
            ItemSend.hide();
            ItemUnblock.hide();
            ItemBlock.show();

            switch (Data.GetType())
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
            case Storage::Models::FriendType::SUGGESTED:
                ItemSend.show();
                break;
            case Storage::Models::FriendType::BLOCKED:
                ItemBlock.hide();
                ItemUnblock.show();
                break;
            }

            Container.popup_at_widget(&TargetWidget, Gdk::GRAVITY_SOUTH_WEST, Gdk::GRAVITY_NORTH_WEST, nullptr);
        }

    private:
        void Construct() {
            ItemChat.signal_activate().connect([this]() { OnAction(ClickAction::CHAT); });
            ItemRemove.signal_activate().connect([this]() { OnAction(ClickAction::REMOVE_FRIEND); });
            ItemAccept.signal_activate().connect([this]() { OnAction(ClickAction::ACCEPT_REQUEST); });
            ItemDecline.signal_activate().connect([this]() { OnAction(ClickAction::DECLINE_REQUEST); });
            ItemCancel.signal_activate().connect([this]() { OnAction(ClickAction::CANCEL_REQUEST); });
            ItemSend.signal_activate().connect([this]() { OnAction(ClickAction::SEND_REQUEST); });
            ItemNickname.signal_activate().connect([this]() { OnAction(ClickAction::SET_NICKNAME); });
            ItemBlock.signal_activate().connect([this]() { OnAction(ClickAction::BLOCK_USER); });
            ItemUnblock.signal_activate().connect([this]() { OnAction(ClickAction::UNBLOCK_USER); });
            ItemCopyId.signal_activate().connect([this]() { OnAction(ClickAction::COPY_USER_ID); });

            ItemChat.set_label("Message");
            ItemRemove.set_label("Remove Friend");
            ItemNickname.set_label("Set Nickname");
            ItemAccept.set_label("Accept Request");
            ItemDecline.set_label("Decline Request");
            ItemCancel.set_label("Cancel Request");
            ItemSend.set_label("Send Friend Request");
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
            Container.add(ItemSend);
            Container.add(ItemSeparator);
            Container.add(ItemBlock);
            Container.add(ItemUnblock);
            Container.add(ItemCopyId);

            Container.show_all();
        }

        std::function<void(ClickAction)> OnAction;

        Gtk::Menu Container;

        // Accepted
        Gtk::MenuItem ItemChat;
        Gtk::MenuItem ItemRemove;
        Gtk::MenuItem ItemNickname;

        // Inbound Pending
        Gtk::MenuItem ItemAccept;
        Gtk::MenuItem ItemDecline;

        // Outbound Pending
        Gtk::MenuItem ItemCancel;

        // Suggested
        Gtk::MenuItem ItemSend;

        // Always
        Gtk::SeparatorMenuItem ItemSeparator;

        // Not Blocked
        Gtk::MenuItem ItemBlock;

        // Blocked
        Gtk::MenuItem ItemUnblock;

        // Always
        Gtk::MenuItem ItemCopyId;
    };
}
