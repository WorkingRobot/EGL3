#pragma once

#include "../../storage/models/Friend.h"
#include "../../utils/SlotHolder.h"
#include "../../web/epic/friends/FriendsClient.h"
#include "../../widgets/FriendItemMenu.h"
#include "../Game/Game.h"
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

        ~FriendsModule();

    private:
        void OnPresenceUpdate(const Web::Epic::Friends::Presence& Presence);

        void OnChatReceived(const std::string& AccountId, const std::string& Message);

        void OnFriendEvent(const std::string& AccountId, Web::Epic::Friends::FriendEventType Event);

        void OnPartyInvite(const std::string& InviterId);

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

        void FriendsUpdate();

        Login::AuthModule& Auth;
        Game::GameModule& Game;
        ImageCacheModule& ImageCache;
        AsyncFFModule& AsyncFF;
        SysTrayModule& SysTray;

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

        Gtk::Stack& MainStack;
        Gtk::Widget& MainStackFriends;

        Utils::SlotHolder SlotViewFriends;
        Utils::SlotHolder SlotAddFriend;
        Utils::SlotHolder SlotAddFriendSend;
        Utils::SlotHolder SlotSetNickname;

        std::optional<Web::Epic::Friends::FriendsClient> FriendsClient;

        std::future<void> UpdateTask;
        Glib::Dispatcher UpdateUIDispatcher;
        std::atomic<bool> UpdateCurrentlyRunning = false;

        std::mutex ItemDataMutex;
        Web::ErrorData::Status ItemDataError;

        std::mutex FriendUpdateMutex;
        std::vector<std::pair<std::string, Web::Epic::Friends::FriendEventType>> FriendUpdateData;
        Glib::Dispatcher FriendUpdateDispatcher;

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