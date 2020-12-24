#pragma once

#include "../storage/models/Friend.h"
#include "../utils/GladeBuilder.h"
#include "../web/epic/EpicClientAuthed.h"
#include "../web/xmpp/XmppClient.h"
#include "../widgets/AsyncImageKeyed.h"
#include "../widgets/CurrentUserItem.h"
#include "../widgets/FriendItem.h"
#include "../widgets/FriendItemMenu.h"
#include "BaseModule.h"
#include "ModuleList.h"
#include "Authorization.h"

#include <gtkmm.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Modules {
	using namespace Web::Xmpp;
	using namespace Storage::Models;

	class FriendsModule : public BaseModule {
	public:
		FriendsModule(ModuleList& Modules, const Utils::GladeBuilder& Builder) :
			ImageCache(Modules.GetModule<ImageCacheModule>()),
			AsyncFF(Modules.GetModule<AsyncFFModule>()),
			Box(Builder.GetWidget<Gtk::ListBox>("FriendsListBox")),
			CurrentUserContainer(Builder.GetWidget<Gtk::Box>("FriendsCurrentUserContainer")),
			CurrentUserModel(Friend::ConstructCurrent),
			CurrentUserWidget(CurrentUserModel, ImageCache),
			KairosMenu(Builder.GetWidget<Gtk::Window>("FriendsKairosMenu")),
			KairosAvatarBox(Builder.GetWidget<Gtk::FlowBox>("FriendsAvatarFlow")),
			KairosBackgroundBox(Builder.GetWidget<Gtk::FlowBox>("FriendsBackgroundFlow")),
			KairosStatusBox(Builder.GetWidget<Gtk::FlowBox>("FriendsStatusFlow")),
			FriendMenu([this](auto Action, const auto& Friend) { OnFriendAction(Action, Friend); })
		{
			Box.set_sort_func([](Gtk::ListBoxRow* A, Gtk::ListBoxRow* B) {
				auto APtr = (Widgets::FriendItem*)A->get_child()->get_data("EGL3_FriendBase");
				auto BPtr = (Widgets::FriendItem*)B->get_child()->get_data("EGL3_FriendBase");

				EGL3_ASSERT(APtr && BPtr, "Widgets aren't of type FriendItem");

				// Undocumented _Value, but honestly, it's fine
				return (*BPtr <=> *APtr)._Value;
			});
			ReSortBoxDispatcher.connect([this]() { Box.invalidate_sort(); });

			KairosMenu.signal_show().connect([this]() {
				KairosMenuFocused = false;

				KairosAvatarBox.foreach([this](Gtk::Widget& WidgetBase) {
					auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
					auto Data = (Widgets::AsyncImageKeyed<std::string>*)Widget.get_child()->get_data("EGL3_ImageBase");
					if (Data->GetKey() == CurrentUserModel.Get().GetKairosAvatar()) {
						KairosAvatarBox.select_child(Widget);
					}
				});
				
				KairosBackgroundBox.foreach([this](Gtk::Widget& WidgetBase) {
					auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
					auto Data = (Widgets::AsyncImageKeyed<std::string>*)Widget.get_child()->get_data("EGL3_ImageBase");
					if (Data->GetKey() == CurrentUserModel.Get().GetKairosBackground()) {
						KairosBackgroundBox.select_child(Widget);
					}
				});

				KairosStatusBox.foreach([this](Gtk::Widget& WidgetBase) {
					auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
					auto Data = (Widgets::AsyncImageKeyed<Json::ShowStatus>*)Widget.get_child()->get_data("EGL3_ImageBase");
					if (Data->GetKey() == CurrentUserModel.Get<FriendCurrent>().GetShowStatus()) {
						KairosStatusBox.select_child(Widget);
					}
				});
			});
			KairosMenu.signal_focus_out_event().connect([this](GdkEventFocus* evt) {
				if (!KairosMenuFocused) {
					// This is always called once for some reason
					// The first event is a fake
					KairosMenuFocused = true;
				}
				else {
					KairosMenu.hide();
				}
				return false;
			});

			KairosAvatarBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
				auto Data = (Widgets::AsyncImageKeyed<std::string>*)child->get_child()->get_data("EGL3_ImageBase");
				if (Data->GetKey() == CurrentUserModel.Get().GetKairosAvatar()) {
					return;
				}

				CurrentUserModel.Get().SetKairosAvatar(Data->GetKey());
				XmppClient->SetPresence(CurrentUserModel.Get<FriendCurrent>().BuildPresence());

				UpdateKairosAvatarTask = std::async(std::launch::async, [this]() {
					auto UpdateResp = LauncherClient->UpdateAccountSetting("avatar", CurrentUserModel.Get().GetKairosAvatar());
					if (UpdateResp.HasError()) {
						printf("Avatar resp: %d\n", UpdateResp.GetErrorCode());
					}
				});
			});

			KairosBackgroundBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
				auto Data = (Widgets::AsyncImageKeyed<std::string>*)child->get_child()->get_data("EGL3_ImageBase");
				if (Data->GetKey() == CurrentUserModel.Get().GetKairosBackground()) {
					return;
				}

				CurrentUserModel.Get().SetKairosBackground(Data->GetKey());
				XmppClient->SetPresence(CurrentUserModel.Get<FriendCurrent>().BuildPresence());

				UpdateKairosBackgroundTask = std::async(std::launch::async, [this]() {
					auto UpdateResp = LauncherClient->UpdateAccountSetting("avatarBackground", CurrentUserModel.Get().GetKairosBackground());
					if (UpdateResp.HasError()) {
						printf("Background resp: %d\n", UpdateResp.GetErrorCode());
					}
				});
			});

			KairosStatusBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
				auto Data = (Widgets::AsyncImageKeyed<Json::ShowStatus>*)child->get_child()->get_data("EGL3_ImageBase");
				if (Data->GetKey() == CurrentUserModel.Get<FriendCurrent>().GetShowStatus()) {
					return;
				}

				CurrentUserModel.Get<FriendCurrent>().SetShowStatus(Data->GetKey());
				XmppClient->SetPresence(CurrentUserModel.Get<FriendCurrent>().BuildPresence());
			});

			CurrentUserModel.Get().SetUpdateCallback([this](const auto& Status) { CurrentUserWidget.Update(); });
			CurrentUserContainer.pack_start(CurrentUserWidget, true, true);
			CurrentUserWidget.SetAsCurrentUser(KairosMenu);

			UpdateUIDispatcher.connect([this]() { UpdateUI(); });
			FriendsRealizeDispatcher.connect([this]() { FriendsRealize(); });

			auto& AuthModule = Modules.GetModule<AuthorizationModule>();
			AuthModule.AuthChanged.connect(sigc::mem_fun(*this, &FriendsModule::OnAuthChanged));

			{
				CurrentUserModel.Get<FriendCurrent>().SetDisplayStatus("Using EGL3");
				CurrentUserModel.Get<FriendCurrent>().SetShowStatus(Json::ShowStatus::Online);
			}

			{
				KairosStatusBox.foreach([this](Gtk::Widget& Widget) { KairosStatusBox.remove(Widget); });


				static const std::initializer_list<Json::ShowStatus> Statuses{ Json::ShowStatus::Online, Json::ShowStatus::Away, Json::ShowStatus::ExtendedAway, Json::ShowStatus::DoNotDisturb, Json::ShowStatus::Chat };

				StatusWidgets.clear();
				StatusWidgets.reserve(Statuses.size());
				for (auto Status : Statuses) {
					auto& Widget = StatusWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<Json::ShowStatus>>(Status, 48, 48, &Json::ShowStatusToUrl, ImageCache));
					KairosStatusBox.add(*Widget);
				}

				KairosStatusBox.show_all_children();
			}
		}

		~FriendsModule() {
			std::scoped_lock Guard(ItemDataMutex);
			while (UpdateCurrentlyRunning) {
				UpdateCurrentlyRunning.wait(true);
			}
		}

	private:
		void OnPresenceUpdate(const std::string& AccountId, Web::Xmpp::Json::Presence&& Presence) {
			if (AccountId != LauncherClient->AuthData.AccountId) {
				std::lock_guard Guard(ItemDataMutex);

				auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&AccountId](const auto& Friend) {
					if (Friend->GetType() == FriendType::NORMAL) {
						return Friend->Get<FriendReal>().GetAccountId() == AccountId;
					}
					return false;
				});
				if (FriendItr != FriendsData.end()) {
					(*FriendItr)->Get<FriendReal>().UpdatePresence(std::move(Presence));
				}
			}
			else {
				CurrentUserModel.Get<FriendCurrent>().UpdatePresence(std::move(Presence));
			}
		}

		void OnFriendshipRequested(Messages::FriendshipRequest&& NewRequest) {
			std::lock_guard Lock(FriendsRealizeMutex);
			FriendsRealizeData.emplace_back(std::move(NewRequest));

			FriendsRealizeDispatcher.emit();
		}

		void OnFriendshipRemoved(Messages::FriendshipRemove&& NewRemoval) {
			std::lock_guard Lock(FriendsRealizeMutex);
			FriendsRealizeData.emplace_back(std::move(NewRemoval));

			FriendsRealizeDispatcher.emit();
		}

		void OnUserBlocklistUpdated(Messages::UserBlocklistUpdate&& NewUpdate) {
			std::lock_guard Lock(FriendsRealizeMutex);
			FriendsRealizeData.emplace_back(std::move(NewUpdate));

			FriendsRealizeDispatcher.emit();
		}

		void OnAuthChanged(Web::Epic::EpicClientAuthed& FNClient, Web::Epic::EpicClientAuthed& LauncherClient) {
			EGL3_ASSERT(LauncherClient.AuthData.AccountId.has_value(), "Launcher client does not have an attached account id");

			CurrentUserModel.Get<FriendCurrent>().SetCurrentUserData(LauncherClient.AuthData.AccountId.value(), LauncherClient.AuthData.DisplayName.value());
			XmppClient.emplace(
				LauncherClient.AuthData.AccountId.value(), LauncherClient.AuthData.AccessToken,
				Callbacks {
					[this](const auto& A, auto&& B) { OnPresenceUpdate(A, std::move(B)); },
					[this](auto&& A) { OnFriendshipRequested(std::move(A)); },
					[this](auto&& A) { OnFriendshipRemoved(std::move(A)); },
					[this](auto&& A) { OnUserBlocklistUpdated(std::move(A)); },
				}
			);
			this->LauncherClient = &LauncherClient;

			UpdateAsync();
		}

		void OnFriendAction(Widgets::FriendItemMenu::ClickAction Action, const Friend& FriendData) {
			auto& Friend = FriendData.Get();
			printf("%s ", Friend.GetDisplayName().c_str());
			switch (Action)
			{
			case Widgets::FriendItemMenu::ClickAction::CHAT:
				printf("chat\n");
				break;
			case Widgets::FriendItemMenu::ClickAction::ACCEPT_REQUEST:
				AsyncFF.EnqueueCallback([this](auto& AccountId) { LauncherClient->AddFriend(AccountId); }, []() {printf("Added User!\n"); }, Friend.GetAccountId());
				printf("send req\n");
				break;
			case Widgets::FriendItemMenu::ClickAction::REMOVE_FRIEND:
			case Widgets::FriendItemMenu::ClickAction::DECLINE_REQUEST:
			case Widgets::FriendItemMenu::ClickAction::CANCEL_REQUEST:
				AsyncFF.EnqueueCallback([this](auto& AccountId) { LauncherClient->RemoveFriend(AccountId); }, []() {printf("Removed User!\n"); }, Friend.GetAccountId());
				printf("cancel req\n");
				break;
			case Widgets::FriendItemMenu::ClickAction::SET_NICKNAME:
				AsyncFF.EnqueueCallback([this](auto& AccountId) { LauncherClient->SetFriendAlias(AccountId, "yo"); }, []() {printf("Nicknamed User!\n"); }, Friend.GetAccountId());
				printf("set nick\n");
				break;
			case Widgets::FriendItemMenu::ClickAction::BLOCK_USER:
				AsyncFF.EnqueueCallback([this](auto& AccountId) { LauncherClient->BlockUser(AccountId); }, []() {printf("Blocked User!\n"); }, Friend.GetAccountId());
				printf("block\n");
				break;
			case Widgets::FriendItemMenu::ClickAction::UNBLOCK_USER:
				AsyncFF.EnqueueCallback([this](auto& AccountId) { LauncherClient->UnblockUser(AccountId); }, []() {printf("Unblocked User!\n"); }, Friend.GetAccountId());
				printf("unblock\n");
				break;
			case Widgets::FriendItemMenu::ClickAction::COPY_USER_ID:
				printf("copy id\n");
				break;
			}
		}

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

				auto FriendsResp = LauncherClient->GetFriendsSummary();
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
							FriendsData.emplace_back(std::make_unique<Storage::Models::Friend>(Friend::ConstructNormal, Friend));
							AccountIds.emplace_back(Friend.AccountId);
						}
						for (auto& Friend : FriendsResp->Incoming) {
							FriendsData.emplace_back(std::make_unique<Storage::Models::Friend>(Friend::ConstructInbound, Friend));
							AccountIds.emplace_back(Friend.AccountId);
						}
						for (auto& Friend : FriendsResp->Outgoing) {
							FriendsData.emplace_back(std::make_unique<Storage::Models::Friend>(Friend::ConstructOutbound, Friend));
							AccountIds.emplace_back(Friend.AccountId);
						}
						for (auto& Friend : FriendsResp->Blocklist) {
							FriendsData.emplace_back(std::make_unique<Storage::Models::Friend>(Friend::ConstructBlocked, Friend));
							AccountIds.emplace_back(Friend.AccountId);
						}

						std::vector<std::string> SendingAccountIds;
						// The maximum is actually 100, but Fortnite is nice and uses 50 instead
						SendingAccountIds.reserve(std::min<size_t>(50, AccountIds.size()));
						size_t SentAccountIds = 0;
						while (AccountIds.size() != SentAccountIds) {
							auto AccountAmt = std::min<size_t>(50, AccountIds.size() - SentAccountIds);
							std::move(AccountIds.begin() + SentAccountIds, AccountIds.begin() + SentAccountIds + AccountAmt, std::back_inserter(SendingAccountIds));

							auto FriendsKairosResp = LauncherClient->GetSettingsForAccounts(SendingAccountIds, { "avatar", "avatarBackground" });

							SendingAccountIds.clear();
							SentAccountIds += AccountAmt;
							if (FriendsKairosResp.HasError()) {
								ItemDataError = FriendsKairosResp.GetErrorCode();
								break;
							}

							for (auto& AccountSetting : FriendsKairosResp->Values) {
								if (AccountSetting.AccountId != LauncherClient->AuthData.AccountId.value()) {
									auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&AccountSetting](const auto& Friend) { return Friend->Get().GetAccountId() == AccountSetting.AccountId; });
									if (FriendItr != FriendsData.end()) {
										(*FriendItr)->Get().UpdateAccountSetting(AccountSetting);
									}
								}
								else {
									CurrentUserModel.Get().UpdateAccountSetting(AccountSetting);
								}
							}

							// At this point, you should be able to set a presence
							XmppClient->SetPresence(CurrentUserModel.Get<FriendCurrent>().BuildPresence());
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
					auto& Widget = FriendsWidgets.emplace_back(std::make_unique<Widgets::FriendItem>(*Friend, ImageCache));
					Widget->SetContextMenu(FriendMenu);
					Friend->Get().SetUpdateCallback([&, this](const auto& Status) { Widget->Update(); ReSortBoxDispatcher.emit(); });
					Box.add(*Widget);
				}

				Box.show_all_children();
			}

			// KairosAvatarsResp handling
			{
				KairosAvatarBox.foreach([this](Gtk::Widget& Widget) { KairosAvatarBox.remove(Widget); });

				AvatarsWidgets.clear();
				AvatarsWidgets.reserve(AvatarsData.size());

				for (auto& Avatar : AvatarsData) {
					auto& Widget = AvatarsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Avatar, 64, 64, &Json::PresenceKairosProfile::GetKairosAvatarUrl, ImageCache));
					KairosAvatarBox.add(*Widget);
				}

				KairosAvatarBox.show_all_children();
			}

			// KairosBackgroundsResp handling
			{
				KairosBackgroundBox.foreach([this](Gtk::Widget& Widget) { KairosBackgroundBox.remove(Widget); });

				BackgroundsWidgets.clear();
				BackgroundsWidgets.reserve(BackgroundsData.size());

				for (auto& Background : BackgroundsData) {
					auto& Widget = BackgroundsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Background, 64, 64, &Json::PresenceKairosProfile::GetKairosBackgroundUrl, ImageCache));
					KairosBackgroundBox.add(*Widget);
				}

				KairosBackgroundBox.show_all_children();
			}

			UpdateCurrentlyRunning = false;
			UpdateCurrentlyRunning.notify_all();
		}

		void FriendsRealize() {
			std::lock_guard Guard(FriendsRealizeMutex);

			for (auto& ActionV : FriendsRealizeData) {
				std::visit([this](auto&& Action) {
					enum ActionEnum : uint8_t {
						UNKNOWN,
						REQUEST_ACCEPT,
						REQUEST_INBOUND,
						REQUEST_OUTBOUND,
						REMOVE,
						BLOCK,
						UNBLOCK
					};
					ActionEnum ActionType = UNKNOWN;
					std::string* AccountId;

					using T = std::decay_t<decltype(Action)>;
					if constexpr (std::is_same_v<T, Messages::FriendshipRequest>) {
						if (Action.From == LauncherClient->AuthData.AccountId) {
							AccountId = &Action.To;
							if (Action.Reason == Messages::FriendshipRequest::ReasonEnum::ACCEPTED) {
								ActionType = REQUEST_ACCEPT;
							}
							else if (Action.Reason == Messages::FriendshipRequest::ReasonEnum::PENDING) {
								ActionType = REQUEST_OUTBOUND;
							}
						}
						else {
							AccountId = &Action.From;
							if (Action.Reason == Messages::FriendshipRequest::ReasonEnum::ACCEPTED) {
								ActionType = REQUEST_ACCEPT;
							}
							else if (Action.Reason == Messages::FriendshipRequest::ReasonEnum::PENDING) {
								ActionType = REQUEST_INBOUND;
							}
						}
					}
					else if constexpr (std::is_same_v<T, Messages::FriendshipRemove>) {
						if (Action.From == LauncherClient->AuthData.AccountId) {
							AccountId = &Action.To;
						}
						else {
							AccountId = &Action.From;
						}
						ActionType = REMOVE;
					}
					else if constexpr (std::is_same_v<T, Messages::UserBlocklistUpdate>) {
						if (Action.Status == Messages::UserBlocklistUpdate::StatusEnum::BLOCKED) {
							ActionType = BLOCK;
						}
						else if (Action.Status == Messages::UserBlocklistUpdate::StatusEnum::UNBLOCKED) {
							ActionType = UNBLOCK;
						}
						AccountId = &Action.Account;
					}

					if (ActionType == UNKNOWN) {
						return;
					}

					std::unique_ptr<Friend>* FriendPtr;
					auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&](auto& Friend) {
						return *AccountId == Friend->Get().GetAccountId();
					});
					if (FriendItr != FriendsData.end()) {
						FriendPtr = &*FriendItr;
					}
					else {
						FriendPtr = &FriendsData.emplace_back(std::make_unique<Friend>(Friend::ConstructInvalid, *AccountId, ""));

						(**FriendPtr).InitializeAccountSettings(*LauncherClient, AsyncFF);

						auto& Widget = FriendsWidgets.emplace_back(std::make_unique<Widgets::FriendItem>(**FriendPtr, ImageCache));
						Widget->SetContextMenu(FriendMenu);
						(*FriendPtr)->Get().SetUpdateCallback([&, this](const auto& Status) { Widget->Update(); ReSortBoxDispatcher.emit(); });
						Box.add(*Widget);
					}

					auto& Friend = **FriendPtr;

					switch (ActionType)
					{
					case REQUEST_ACCEPT:
						Friend.SetAddFriendship(*LauncherClient, AsyncFF);
						break;
					case REQUEST_INBOUND:
						Friend.SetRequestInbound(*LauncherClient, AsyncFF);
						break;
					case REQUEST_OUTBOUND:
						Friend.SetRequestOutbound(*LauncherClient, AsyncFF);
						break;
					case REMOVE:
						Friend.SetRemoveFriendship(*LauncherClient, AsyncFF);
						break;
					case BLOCK:
						Friend.SetBlocked(*LauncherClient, AsyncFF);
						break;
					case UNBLOCK:
						Friend.SetUnblocked(*LauncherClient, AsyncFF);
						break;
					}
				}, ActionV);
			}

			FriendsRealizeData.clear();
		}

		ImageCacheModule& ImageCache;
		AsyncFFModule& AsyncFF;

		Glib::Dispatcher ReSortBoxDispatcher;
		Gtk::ListBox& Box;

		Widgets::FriendItemMenu FriendMenu;

		Gtk::Box& CurrentUserContainer;
		Friend CurrentUserModel;
		Widgets::CurrentUserItem CurrentUserWidget;

		bool KairosMenuFocused = false;
		Gtk::Window& KairosMenu;
		Gtk::FlowBox& KairosAvatarBox;
		Gtk::FlowBox& KairosBackgroundBox;
		Gtk::FlowBox& KairosStatusBox;

		Web::Epic::EpicClientAuthed* LauncherClient;
		std::optional<XmppClient> XmppClient;


		std::future<void> UpdateKairosAvatarTask;
		std::future<void> UpdateKairosBackgroundTask;

		std::future<void> UpdateTask;
		Glib::Dispatcher UpdateUIDispatcher;
		std::atomic<bool> UpdateCurrentlyRunning = false;

		std::mutex ItemDataMutex;
		Web::BaseClient::ErrorCode ItemDataError;

		std::mutex FriendsRealizeMutex;
		std::vector<std::variant<Messages::FriendshipRequest, Messages::FriendshipRemove, Messages::UserBlocklistUpdate>> FriendsRealizeData;
		Glib::Dispatcher FriendsRealizeDispatcher;

		std::vector<std::unique_ptr<Friend>> FriendsData;
		std::vector<std::unique_ptr<Widgets::FriendItem>> FriendsWidgets;

		std::vector<std::string> AvatarsData;
		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<std::string>>> AvatarsWidgets;

		std::vector<std::string> BackgroundsData;
		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<std::string>>> BackgroundsWidgets;

		std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<Json::ShowStatus>>> StatusWidgets;
	};
}