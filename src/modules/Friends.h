#pragma once

#include "../storage/models/Friend.h"
#include "../utils/GladeBuilder.h"
#include "../web/epic/EpicClientAuthed.h"
#include "../web/xmpp/XmppClient.h"
#include "../widgets/CurrentUserItem.h"
#include "../widgets/FriendItem.h"
#include "../widgets/FriendItemMenu.h"
#include "BaseModule.h"
#include "ModuleList.h"
#include "AsyncFF.h"
#include "FriendsList.h"
#include "FriendsOptions.h"
#include "KairosMenu.h"

#include <gtkmm.h>

namespace EGL3::Modules {
	class FriendsModule : public BaseModule {
	public:
		FriendsModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

	private:
		void OnPresenceUpdate(const std::string& AccountId, Web::Xmpp::Json::Presence&& Presence);

		void OnSystemMessage(Web::Xmpp::Messages::SystemMessage&& NewMessage);

		void OnAuthChanged(Web::Epic::EpicClientAuthed& FNClient, Web::Epic::EpicClientAuthed& LauncherClient);

		void OnFriendAction(Widgets::FriendItemMenu::ClickAction Action, const Storage::Models::Friend& FriendData);

		void OnOpenViewFriends();

		void OnOpenAddFriendPage();

		void OnOpenSetNicknamePage(const Storage::Models::Friend& FriendData);

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

		AuthorizationModule& Auth;
		ImageCacheModule& ImageCache;
		AsyncFFModule& AsyncFF;
		FriendsOptionsModule& Options;
		KairosMenuModule& KairosMenu;
		FriendsListModule& FriendsList;

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