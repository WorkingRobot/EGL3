#pragma once

#include "../storage/models/Friend.h"
#include "../storage/models/StoredFriendData.h"
#include "../utils/GladeBuilder.h"
#include "../web/epic/EpicClientAuthed.h"
#include "../web/xmpp/XmppClient.h"
#include "../widgets/AsyncImageKeyed.h"
#include "../widgets/CurrentUserItem.h"
#include "../widgets/FriendItem.h"
#include "../widgets/FriendItemMenu.h"
#include "BaseModule.h"
#include "ModuleList.h"
#include "AsyncFF.h"

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

		void OnSendFriendRequest();

		void SendFriendRequest(const Web::Epic::Responses::GetAccounts::Account& Account);

		enum class FriendRequestStatusType : uint8_t {
			Success,
			Ratelimited,
			Failure
		};

		void DisplaySendFriendRequestStatus();

		void UpdateSelection();

		void ResortWidget(Widgets::FriendItem& Widget);

		void UpdateAsync();
		
		void UpdateUI();

		void FriendsRealize();

		void ResortBox();

		ImageCacheModule& ImageCache;
		AsyncFFModule& AsyncFF;
		Storage::Models::StoredFriendData& StorageData;

		Gtk::Button& ViewFriendsBtn;
		Gtk::Button& AddFriendBtn;
		Gtk::CheckMenuItem& CheckFriendsOffline;
		Gtk::CheckMenuItem& CheckFriendsOutgoing;
		Gtk::CheckMenuItem& CheckFriendsIncoming;
		Gtk::CheckMenuItem& CheckFriendsBlocked;
		Gtk::CheckMenuItem& CheckDeclineReqs;
		Gtk::CheckMenuItem& CheckProfanity;

		Gtk::Button& AddFriendSendBtn;
		Gtk::Entry& AddFriendEntry;
		Gtk::Label& AddFriendStatus;

		Gtk::Stack& SwitchStack;
		Gtk::Widget& SwitchStackPage0;
		Gtk::Widget& SwitchStackPage1;

		Gtk::ListBox& Box;

		Widgets::FriendItemMenu FriendMenu;

		Gtk::Box& CurrentUserContainer;
		Storage::Models::Friend CurrentUserModel;
		Widgets::CurrentUserItem CurrentUserWidget;

		bool KairosMenuFocused = false;
		Gtk::Window& KairosMenu;
		Gtk::FlowBox& KairosAvatarBox;
		Gtk::FlowBox& KairosBackgroundBox;
		Gtk::FlowBox& KairosStatusBox;
		Gtk::Entry& KairosStatusEntry;
		Gtk::Button& KairosStatusEditBtn;

		Web::Epic::EpicClientAuthed* LauncherClient;
		std::optional<Web::Xmpp::XmppClient> XmppClient;


		std::future<void> UpdateKairosAvatarTask;
		std::future<void> UpdateKairosBackgroundTask;

		std::future<void> UpdateTask;
		Glib::Dispatcher UpdateUIDispatcher;
		std::atomic<bool> UpdateCurrentlyRunning = false;

		std::mutex ItemDataMutex;
		Web::ErrorData::Status ItemDataError;

		std::mutex FriendsRealizeMutex;
		std::vector<Web::Xmpp::Messages::SystemMessage> FriendsRealizeData;
		Glib::Dispatcher FriendsRealizeDispatcher;

		std::mutex ResortBoxMutex;
		std::vector<std::reference_wrapper<Widgets::FriendItem>> ResortBoxData;
		Glib::Dispatcher ResortBoxDispatcher;

		std::vector<std::unique_ptr<Storage::Models::Friend>> FriendsData;
		std::vector<std::unique_ptr<Widgets::FriendItem>> FriendsWidgets;

		std::future<void> FriendRequestTask;
		FriendRequestStatusType FriendRequestStatus;
		std::string FriendRequestStatusText;
		Glib::Dispatcher FriendRequestDispatcher;

		std::vector<std::string> AvatarsData;
		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<std::string>>> AvatarsWidgets;

		std::vector<std::string> BackgroundsData;
		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<std::string>>> BackgroundsWidgets;

		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<Web::Xmpp::Json::ShowStatus>>> StatusWidgets;
	};
}