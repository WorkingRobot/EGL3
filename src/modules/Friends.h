#pragma once

#include "../storage/models/Friend.h"
#include "../utils/GladeBuilder.h"
#include "../web/xmpp/XmppClient.h"
#include "../widgets/AsyncImageKeyed.h"
#include "../widgets/FriendItem.h"
#include "BaseModule.h"
#include "ModuleList.h"

#include <gtkmm.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Modules {
	using namespace Web::Xmpp;

	class FriendsModule : public BaseModule {
	public:
		FriendsModule(ModuleList& Modules, const Utils::GladeBuilder& Builder) :
			ImageCache(Modules.GetModule<ImageCacheModule>()),
			Box(Builder.GetWidget<Gtk::ListBox>("FriendsListBox")),
			CurrentUserContainer(Builder.GetWidget<Gtk::Box>("FriendsCurrentUserContainer")),
			CurrentUserModel(),
			CurrentUserWidget(CurrentUserModel, ImageCache),
			KairosMenu(Builder.GetWidget<Gtk::Window>("FriendsKairosMenu")),
			KairosAvatarBox(Builder.GetWidget<Gtk::FlowBox>("FriendsAvatarFlow")),
			KairosBackgroundBox(Builder.GetWidget<Gtk::FlowBox>("FriendsBackgroundFlow"))
		{
			Box.set_sort_func([](Gtk::ListBoxRow* A, Gtk::ListBoxRow* B) {
				auto APtr = (Widgets::FriendItem*)A->get_child()->get_data("EGL3_FriendBase");
				auto BPtr = (Widgets::FriendItem*)B->get_child()->get_data("EGL3_FriendBase");

				EGL3_ASSERT(APtr && BPtr, "Widgets aren't of type FriendItem");

				// Undocumented _Value, but honestly, it's fine
				return (*APtr <=> *BPtr)._Value;
			});
			ReSortBoxDispatcher.connect([this]() { Box.invalidate_sort(); });

			KairosMenu.signal_show().connect([this]() {
				KairosMenuFocused = false;

				KairosAvatarBox.foreach([this](Gtk::Widget& WidgetBase) {
					auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
					auto Data = (Widgets::AsyncImageKeyed*)Widget.get_child()->get_data("EGL3_ImageBase");
					if (Data->GetKey() == CurrentUserModel.Kairos.Avatar) {
						KairosAvatarBox.select_child(Widget);
					}
				});
				
				KairosBackgroundBox.foreach([this](Gtk::Widget& WidgetBase) {
					auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
					auto Data = (Widgets::AsyncImageKeyed*)Widget.get_child()->get_data("EGL3_ImageBase");
					if (Data->GetKey() == CurrentUserModel.Kairos.Background) {
						KairosBackgroundBox.select_child(Widget);
					}
				});
			});
			KairosMenu.signal_focus_out_event().connect([this](GdkEventFocus* evt) {
				if (!KairosMenuFocused) {
					KairosMenuFocused = true;
				}
				else {
					KairosMenu.hide();
				}
				return false;
			});

			KairosAvatarBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
				auto Data = (Widgets::AsyncImageKeyed*)child->get_child()->get_data("EGL3_ImageBase");
				if (Data->GetKey() == CurrentUserModel.Kairos.Avatar) {
					return;
				}

				CurrentUserModel.Kairos.Avatar = Data->GetKey();
				XmppClient->SetPresence(CurrentUserModel.GetStatusData());

				UpdateKairosAvatarTask = std::async(std::launch::async, [this]() {
					auto UpdateResp = LauncherClient->UpdateAccountSetting("avatar", CurrentUserModel.Kairos.Avatar);
					if (UpdateResp.HasError()) {
						printf("Avatar resp: %d\n", UpdateResp.GetErrorCode());
					}
				});
			});

			KairosBackgroundBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
				auto Data = (Widgets::AsyncImageKeyed*)child->get_child()->get_data("EGL3_ImageBase");
				if (Data->GetKey() == CurrentUserModel.Kairos.Background) {
					return;
				}

				CurrentUserModel.Kairos.Background = Data->GetKey();
				XmppClient->SetPresence(CurrentUserModel.GetStatusData());

				UpdateKairosBackgroundTask = std::async(std::launch::async, [this]() {
					auto UpdateResp = LauncherClient->UpdateAccountSetting("avatarBackground", CurrentUserModel.Kairos.Background);
					if (UpdateResp.HasError()) {
						printf("Background resp: %d\n", UpdateResp.GetErrorCode());
					}
				});
			});

			CurrentUserModel.Updated = [this](const auto& Status) { CurrentUserWidget.Update(); ReSortBoxDispatcher.emit(); };
			CurrentUserContainer.pack_start(CurrentUserWidget, true, true);
			CurrentUserWidget.SetAsCurrentUser(KairosMenu);

			UpdateUIDispatcher.connect([this]() { UpdateUI(); });

			auto& AuthModule = Modules.GetModule<AuthorizationModule>();
			AuthModule.AuthChanged.connect(sigc::mem_fun(*this, &FriendsModule::OnAuthChanged));

			CurrentUserModel.DisplayStatus = "Using EGL3";
			CurrentUserModel.OnlineStatus = Web::Xmpp::Json::ShowStatus::Online;
		}

		~FriendsModule() {
			std::scoped_lock Guard(ItemDataMutex);
			while (UpdateCurrentlyRunning) {
				UpdateCurrentlyRunning.wait(true);
			}
		}

	private:
		void OnPresenceUpdate(const std::string& AccountId, const Web::Xmpp::Json::StatusData& Status) {
			if (AccountId != LauncherClient->AuthData.AccountId) {
				std::lock_guard Guard(ItemDataMutex);

				auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&AccountId](const auto& Friend) { return Friend.AccountId == AccountId; });
				if (FriendItr != FriendsData.end()) {
					FriendItr->Update(Status);
				}
			}
			else {
				CurrentUserModel.Update(Status);
			}
		}

		void OnAuthChanged(Web::Epic::EpicClientAuthed& FNClient, Web::Epic::EpicClientAuthed& LauncherClient) {
			EGL3_ASSERT(LauncherClient.AuthData.AccountId.has_value(), "Launcher client does not have an attached account id");

			XmppClient.emplace(
				LauncherClient.AuthData.AccountId.value(), LauncherClient.AuthData.AccessToken,
				[this]() { XmppClient->SetPresence(CurrentUserModel.GetStatusData()); },
				[this](const std::string& AccountId, const Web::Xmpp::Json::StatusData& Status) { OnPresenceUpdate(AccountId, Status); }
			);
			this->LauncherClient = &LauncherClient;

			UpdateAsync();
		}

		// TODO: wait until you get a response from json to set the presence avatar
		void UpdateAsync() {
			if (!LauncherClient) {
				return;
			}

			{ // Atomic CAS: if there is no update running, return, otherwise, set the flag and continue
				bool OldValue = UpdateCurrentlyRunning.load();
				do {
					if (OldValue) {
						return;
					}
				} while (!UpdateCurrentlyRunning.compare_exchange_weak(OldValue, true));
			}

			UpdateTask = std::async(std::launch::async, [this]() {
				std::unique_lock ItemDataGuard(ItemDataMutex);

				auto FriendsResp = LauncherClient->GetFriends(true);
				auto KairosAvatarsResp = LauncherClient->GetAvailableSettingValues("avatar");
				auto KairosBackgroundsResp = LauncherClient->GetAvailableSettingValues("avatarBackground");

				if (FriendsResp.HasError()) {
					ItemDataError = FriendsResp.GetErrorCode();
				}
				else if (KairosAvatarsResp.HasError()) {
					ItemDataError = KairosAvatarsResp.GetErrorCode();
				}
				else if (KairosBackgroundsResp.HasError()) {
					ItemDataError = KairosBackgroundsResp.GetErrorCode();
				}
				else {
					// FriendsResp handling
					{
						FriendsData.clear();

						// Chunking account ids to send, maximum 100 ids per request
						std::vector<std::string> AccountIds;
						AccountIds.reserve(FriendsResp->Friends.size() + 1);
						// We've already asserted that LauncherClient has an account id
						AccountIds.emplace_back(LauncherClient->AuthData.AccountId.value());
						for (auto& Friend : FriendsResp->Friends) {
							FriendsData.emplace_back(Friend);
							AccountIds.emplace_back(Friend.AccountId);
						}

						std::vector<std::string> SendingAccountIds;
						// The maximum is actually 100, but Fortnite is nice and uses 50 instead
						SendingAccountIds.reserve(std::min<size_t>(50, AccountIds.size()));
						size_t SentAccountIds = 0;
						while (AccountIds.size() != SentAccountIds) {
							auto AccountAmt = std::min<size_t>(50, AccountIds.size() - SentAccountIds);
							std::move(AccountIds.begin() + SentAccountIds, AccountIds.begin() + SentAccountIds + AccountAmt, std::back_inserter(SendingAccountIds));

							auto FriendsAccResp = LauncherClient->GetAccounts(SendingAccountIds);
							auto FriendsKairosResp = LauncherClient->GetSettingsForAccounts(SendingAccountIds, { "avatar", "avatarBackground" });

							SendingAccountIds.clear();
							SentAccountIds += AccountAmt;
							if (FriendsAccResp.HasError()) {
								ItemDataError = FriendsAccResp.GetErrorCode();
								break;
							}
							else if (FriendsKairosResp.HasError()) {
								ItemDataError = FriendsKairosResp.GetErrorCode();
								break;
							}

							for (auto& Account : FriendsAccResp->Accounts) {
								if (Account.Id != LauncherClient->AuthData.AccountId.value()) {
									auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&Account](const auto& Friend) { return Friend.AccountId == Account.Id; });
									if (FriendItr != FriendsData.end()) {
										FriendItr->Update(Account);
									}
								}
								else {
									CurrentUserModel.Update(Account);
								}
							}

							for (auto& AccountSetting : FriendsKairosResp->Values) {
								if (AccountSetting.AccountId != LauncherClient->AuthData.AccountId.value()) {
									auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&AccountSetting](const auto& Friend) { return Friend.AccountId == AccountSetting.AccountId; });
									if (FriendItr != FriendsData.end()) {
										FriendItr->Update(AccountSetting);
									}
								}
								else {
									CurrentUserModel.Update(AccountSetting);
								}
							}
							// At this point, you should be able to set a presence
						}
					}

					// KairosAvatarsResp and KairosBackgroundsResp handling
					{
						AvatarsData = std::move(KairosAvatarsResp->Values);

						BackgroundsData = std::move(KairosBackgroundsResp->Values);
					}

					ItemDataError = Web::BaseClient::SUCCESS;
				}

				ItemDataGuard.unlock();
				UpdateUIDispatcher.emit();
			});
		}
		
		void UpdateUI() {
			std::lock_guard Guard(ItemDataMutex);

			// FriendsResp handling
			{
				Box.foreach([this](Gtk::Widget& Widget) { Box.remove(Widget); });

				FriendsWidgets.clear();
				FriendsWidgets.reserve(FriendsData.size());

				for (auto& Friend : FriendsData) {
					auto& Widget = FriendsWidgets.emplace_back(std::make_unique<Widgets::FriendItem>(Friend, ImageCache));
					Friend.Updated = [&, this](const auto& Status) { Widget->Update(); ReSortBoxDispatcher.emit(); };
					Box.add(*Widget);
				}

				Box.show_all_children();
			}

			// KairosAvatarsResp handling
			{
				KairosAvatarBox.foreach([this](Gtk::Widget& Widget) { Box.remove(Widget); });

				AvatarsWidgets.clear();
				AvatarsWidgets.reserve(AvatarsData.size());

				for (auto& Avatar : AvatarsData) {
					auto& Widget = AvatarsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed>(Avatar, 64, 64, &Web::Xmpp::Json::KairosProfile::StaticGetAvatarUrl, ImageCache));
					KairosAvatarBox.add(*Widget);
				}

				KairosAvatarBox.show_all_children();
			}

			// KairosBackgroundsResp handling
			{
				KairosBackgroundBox.foreach([this](Gtk::Widget& Widget) { Box.remove(Widget); });

				BackgroundsWidgets.clear();
				BackgroundsWidgets.reserve(BackgroundsData.size());

				for (auto& Background : BackgroundsData) {
					auto& Widget = BackgroundsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed>(Background, 64, 64, &Web::Xmpp::Json::KairosProfile::StaticGetBackgroundUrl, ImageCache));
					KairosBackgroundBox.add(*Widget);
				}

				KairosBackgroundBox.show_all_children();
			}

			UpdateCurrentlyRunning = false;
			UpdateCurrentlyRunning.notify_all();
		}

		Modules::ImageCacheModule& ImageCache;

		Glib::Dispatcher ReSortBoxDispatcher;
		Gtk::ListBox& Box;

		Gtk::Box& CurrentUserContainer;
		Storage::Models::Friend CurrentUserModel;
		Widgets::FriendItem CurrentUserWidget;

		bool KairosMenuFocused = false;
		Gtk::Window& KairosMenu;
		Gtk::FlowBox& KairosAvatarBox;
		Gtk::FlowBox& KairosBackgroundBox;

		Web::Epic::EpicClientAuthed* LauncherClient;
		std::optional<XmppClient> XmppClient;


		std::future<void> UpdateKairosAvatarTask;
		std::future<void> UpdateKairosBackgroundTask;

		std::future<void> UpdateTask;
		Glib::Dispatcher UpdateUIDispatcher;
		std::atomic<bool> UpdateCurrentlyRunning = false;

		std::mutex ItemDataMutex;
		Web::BaseClient::ErrorCode ItemDataError;

		std::vector<Storage::Models::Friend> FriendsData;
		std::vector<std::unique_ptr<Widgets::FriendItem>> FriendsWidgets;

		std::vector<std::string> AvatarsData;
		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed>> AvatarsWidgets;

		std::vector<std::string> BackgroundsData;
		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed>> BackgroundsWidgets;
	};
}