#pragma once

#include "../storage/models/Friend.h"
#include "../utils/Callback.h"

#include <gtkmm.h>

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
            BLOCK_USER,
            UNBLOCK_USER,
            COPY_USER_ID
        };

        explicit FriendItemMenu();

        operator Gtk::Menu&();

        void PopupMenu(const Storage::Models::Friend& Friend, Gtk::Widget& TargetWidget);

        void PopupMenu(const Storage::Models::Friend& Friend, Gtk::Widget& TargetWidget, Gdk::Rectangle& TargetRect);

        Utils::Callback<void(ClickAction, const Storage::Models::Friend&)> OnAction;

    private:
        void Construct();

        void SetupFriendForPopup(const Storage::Models::Friend& Friend);

        const Storage::Models::Friend* SelectedFriend;

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
