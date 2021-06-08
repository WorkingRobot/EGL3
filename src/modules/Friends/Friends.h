#pragma once

#include "../../storage/models/Friend.h"
#include "../../web/xmpp/XmppClient.h"
#include "../../widgets/FriendItemMenu.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "Chat.h"
#include "List.h"
#include "Options.h"
#include "KairosMenu.h"

#include <gtkmm.h>

namespace EGL3::Modules::Friends {
    class FriendsModule : public BaseModule {
    public:
        FriendsModule(ModuleList& Ctx);

    private:
        void OnPresenceUpdate(const std::string& AccountId, Web::Xmpp::Json::Presence&& Presence);

        void OnChatMessage(const std::string& AccountId, std::string&& Message);

        void OnSystemMessage(Web::Xmpp::Messages::SystemMessage&& NewMessage);

        void OnFriendAction(Widgets::FriendItemMenu::ClickAction Action, Storage::Models::Friend& FriendData);

        void OnOpenViewFriends();

        void OnOpenAddFriendPage();

        void OnOpenSetNicknamePage(const Storage::Models::Friend& FriendData);

        void OnOpenChatPage(Storage::Models::Friend& FriendData);

        void OnSendFriendRequest();

        void SendFriendRequest(const Web::Epic::Responses::GetAccounts::Account& Account);

        enum class AsyncWebRequestStatusType : uint8_t {
            Success,
            Ratelimited,
            Failure
        };

        void DisplaySendFriendRequestStatus();

        void OnSetNickname();

        void DisplaySetNicknameStatus();

        void UpdateAsync();
        
        void UpdateUI();

        void FriendsRealize();

        Login::AuthModule& Auth;
        ImageCacheModule& ImageCache;
        AsyncFFModule& AsyncFF;

        OptionsModule& Options;
        KairosMenuModule& KairosMenu;
        ListModule& FriendsList;
        ChatModule& FriendsChat;

        Gtk::Button& ViewFriendsBtn;
        Gtk::Button& AddFriendBtn;

        Gtk::Button& AddFriendSendBtn;
        Gtk::Entry& AddFriendEntry;
        Gtk::Label& AddFriendStatus;

        Gtk::Label& SetNicknameLabel;
        Gtk::Button& SetNicknameBtn;
        Gtk::Entry& SetNicknameEntry;
        Gtk::Label& SetNicknameStatusLabel;

        Gtk::Stack& SwitchStack;
        Gtk::Widget& SwitchStackPage0;
        Gtk::Widget& SwitchStackPage1;
        Gtk::Widget& SwitchStackPage2;
        Gtk::Widget& SwitchStackPage3;

        std::optional<Web::Xmpp::XmppClient> XmppClient;

        std::future<void> UpdateTask;
        Glib::Dispatcher UpdateUIDispatcher;
        std::atomic<bool> UpdateCurrentlyRunning = false;

        std::mutex ItemDataMutex;
        Web::ErrorData::Status ItemDataError;

        std::mutex FriendsRealizeMutex;
        std::vector<Web::Xmpp::Messages::SystemMessage> FriendsRealizeData;
        Glib::Dispatcher FriendsRealizeDispatcher;

        std::future<void> FriendRequestTask;
        AsyncWebRequestStatusType FriendRequestStatus;
        std::string FriendRequestStatusText;
        Glib::Dispatcher FriendRequestDispatcher;

        std::string SetNicknameAccountId;
        std::future<void> SetNicknameTask;
        AsyncWebRequestStatusType SetNicknameStatus;
        std::string SetNicknameStatusText;
        Glib::Dispatcher SetNicknameDispatcher;
    };
}