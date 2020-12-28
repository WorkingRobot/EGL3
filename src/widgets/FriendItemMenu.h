#pragma once

#include "../storage/models/Friend.h"

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

        using Callback = std::function<void(ClickAction, const Storage::Models::Friend&)>;

        FriendItemMenu(const Callback& OnAction);

        operator Gtk::Menu& ();

        void PopupMenu(const Storage::Models::Friend& Friend, Gtk::Widget& TargetWidget);

    private:
        void Construct();

        Callback OnAction;
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
