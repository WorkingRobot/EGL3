#pragma once

#include <gtkmm.h>

#include "../storage/models/Friend.h"

namespace EGL3::Widgets {
    class FriendItemMenu {
    public:
        enum class ClickAction : uint8_t {
            CHAT,
            REMOVE_FRIEND,
            ACCEPT_REQUEST,
            DECLINE_REQUEST,
            CANCEL_REQUEST,
            SEND_REQUEST,
            SET_NICKNAME,
            BLOCK_USER,
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
            ItemAccept.hide();
            ItemDecline.hide();
            ItemCancel.hide();
            ItemSend.hide();
            switch (Data.GetRelationStatus())
            {
            case Storage::Models::Friend::RelationStatus::ACCEPTED:
                ItemChat.show();
                ItemRemove.show();
                break;
            case Storage::Models::Friend::RelationStatus::PENDING:
                switch (Data.GetRelationDirection())
                {
                case Storage::Models::Friend::RelationDirection::INBOUND:
                    ItemAccept.show();
                    ItemDecline.show();
                    break;
                case Storage::Models::Friend::RelationDirection::OUTBOUND:
                    ItemCancel.show();
                    break;
                }
                break;
            case Storage::Models::Friend::RelationStatus::SUGGESTED:
                ItemSend.show();
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
            ItemCopyId.signal_activate().connect([this]() { OnAction(ClickAction::COPY_USER_ID); });

            ItemChat.set_label("Message");
            ItemRemove.set_label("Remove Friend");
            ItemAccept.set_label("Accept Request");
            ItemDecline.set_label("Decline Request");
            ItemCancel.set_label("Cancel Request");
            ItemSend.set_label("Send Friend Request");
            ItemSeparator.set_label("\xe2\xb8\xbb"); // triple em dash
            ItemNickname.set_label("Set Nickname");
            ItemBlock.set_label("Block");
            ItemCopyId.set_label("Copy Account Id");

            Container.get_style_context()->add_class("font-h5");

            Container.add(ItemChat);
            Container.add(ItemRemove);
            Container.add(ItemAccept);
            Container.add(ItemDecline);
            Container.add(ItemCancel);
            Container.add(ItemSend);
            Container.add(ItemSeparator);
            Container.add(ItemNickname);
            Container.add(ItemBlock);
            Container.add(ItemCopyId);

            Container.show_all();
        }

        std::function<void(ClickAction)> OnAction;

        Gtk::Menu Container;

        // Accepted
        Gtk::MenuItem ItemChat;
        Gtk::MenuItem ItemRemove;

        // Inbound Pending
        Gtk::MenuItem ItemAccept;
        Gtk::MenuItem ItemDecline;

        // Outbound Pending
        Gtk::MenuItem ItemCancel;

        // Suggested
        Gtk::MenuItem ItemSend;

        // Always
        Gtk::SeparatorMenuItem ItemSeparator;
        Gtk::MenuItem ItemNickname;
        Gtk::MenuItem ItemBlock;
        Gtk::MenuItem ItemCopyId;
    };
}
