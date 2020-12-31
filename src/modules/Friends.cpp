#include "Friends.h"

#include "../utils/EmitRAII.h"
#include "Authorization.h"

#include <array>
#include <regex>

namespace EGL3::Modules {
    using namespace Web::Xmpp;
    using namespace Storage::Models;

    FriendsModule::FriendsModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
            ImageCache(Modules.GetModule<ImageCacheModule>()),
            AsyncFF(Modules.GetModule<AsyncFFModule>()),
            StorageData(Storage.Get(Storage::Persistent::Key::StoredFriendData)),
            ViewFriendsBtn(Builder.GetWidget<Gtk::Button>("FriendViewFriendsBtn")),
            AddFriendBtn(Builder.GetWidget<Gtk::Button>("FriendsOpenSendRequestBtn")),
            CheckFriendsOffline(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsOfflineCheck")),
            CheckFriendsOutgoing(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsOutgoingCheck")),
            CheckFriendsIncoming(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsIncomingCheck")),
            CheckFriendsBlocked(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsBlockedCheck")),
            CheckDeclineReqs(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsDeclineReqsCheck")),
            CheckProfanity(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsProfanityCheck")),
            AddFriendSendBtn(Builder.GetWidget<Gtk::Button>("FriendsSendRequestBtn")),
            AddFriendEntry(Builder.GetWidget<Gtk::Entry>("FriendsSendRequestEntry")),
            AddFriendStatus(Builder.GetWidget<Gtk::Label>("FriendsSendRequestStatus")),
            SwitchStack(Builder.GetWidget<Gtk::Stack>("FriendsStack")),
            SwitchStackPage0(Builder.GetWidget<Gtk::Widget>("FriendsStackPage0")),
            SwitchStackPage1(Builder.GetWidget<Gtk::Widget>("FriendsStackPage1")),
            Box(Builder.GetWidget<Gtk::ListBox>("FriendsListBox")),
            CurrentUserContainer(Builder.GetWidget<Gtk::Box>("FriendsCurrentUserContainer")),
            CurrentUserModel(Friend::ConstructCurrent),
            CurrentUserWidget(CurrentUserModel, ImageCache),
            KairosMenu(Builder.GetWidget<Gtk::Window>("FriendsKairosMenu")),
            KairosAvatarBox(Builder.GetWidget<Gtk::FlowBox>("FriendsAvatarFlow")),
            KairosBackgroundBox(Builder.GetWidget<Gtk::FlowBox>("FriendsBackgroundFlow")),
            KairosStatusBox(Builder.GetWidget<Gtk::FlowBox>("FriendsStatusFlow")),
            KairosStatusEntry(Builder.GetWidget<Gtk::Entry>("FriendsKairosStatusEntry")),
            KairosStatusEditBtn(Builder.GetWidget<Gtk::Button>("FriendsKairosEditStatusBtn")),
            FriendMenu([this](auto Action, const auto& Friend) { OnFriendAction(Action, Friend); })
        {
            {
                ViewFriendsBtn.signal_clicked().connect([this]() { OnOpenViewFriends(); });

                AddFriendBtn.signal_clicked().connect([this]() { OnOpenAddFriendPage(); });

                CheckFriendsOffline.set_active(StorageData.HasFlag<StoredFriendData::ShowOffline>());
                CheckFriendsOutgoing.set_active(StorageData.HasFlag<StoredFriendData::ShowOutgoing>());
                CheckFriendsIncoming.set_active(StorageData.HasFlag<StoredFriendData::ShowIncoming>());
                CheckFriendsBlocked.set_active(StorageData.HasFlag<StoredFriendData::ShowBlocked>());
                CheckDeclineReqs.set_active(StorageData.HasFlag<StoredFriendData::AutoDeclineReqs>());
                CheckProfanity.set_active(StorageData.HasFlag<StoredFriendData::CensorProfanity>());

                CheckFriendsOffline.signal_toggled().connect([this]() { UpdateSelection(); });
                CheckFriendsOutgoing.signal_toggled().connect([this]() { UpdateSelection(); });
                CheckFriendsIncoming.signal_toggled().connect([this]() { UpdateSelection(); });
                CheckFriendsBlocked.signal_toggled().connect([this]() { UpdateSelection(); });
                CheckDeclineReqs.signal_toggled().connect([this]() { UpdateSelection(); });
                CheckProfanity.signal_toggled().connect([this]() { UpdateSelection(); });
            }

            {
                AddFriendSendBtn.signal_clicked().connect([this]() { OnSendFriendRequest(); });

                FriendRequestDispatcher.connect([this]() { DisplaySendFriendRequestStatus(); });
            }

            {
                Box.set_sort_func([](Gtk::ListBoxRow* A, Gtk::ListBoxRow* B) {
                    auto APtr = (Widgets::FriendItem*)A->get_child()->get_data("EGL3_FriendBase");
                    auto BPtr = (Widgets::FriendItem*)B->get_child()->get_data("EGL3_FriendBase");

                    EGL3_CONDITIONAL_LOG(APtr && BPtr, LogLevel::Critical, "Widgets aren't of type FriendItem");

                    // Undocumented _Value, but honestly, it's fine
                    return (*BPtr <=> *APtr)._Value;
                });
                Box.set_filter_func([this](Gtk::ListBoxRow* Row) {
                    auto Ptr = (Widgets::FriendItem*)Row->get_child()->get_data("EGL3_FriendBase");

                    EGL3_CONDITIONAL_LOG(Ptr, LogLevel::Critical, "Widget isn't of type FriendItem");

                    switch (Ptr->GetData().GetType()) {
                    case FriendType::INVALID:
                        return false;
                    case FriendType::BLOCKED:
                        return StorageData.HasFlag<StoredFriendData::ShowBlocked>();
                    case FriendType::OUTBOUND:
                        return StorageData.HasFlag<StoredFriendData::ShowOutgoing>();
                    case FriendType::INBOUND:
                        return StorageData.HasFlag<StoredFriendData::ShowIncoming>();
                    case FriendType::NORMAL:
                        return StorageData.HasFlag<StoredFriendData::ShowOffline>() || Ptr->GetData().Get<FriendCurrent>().GetShowStatus() != Json::ShowStatus::Offline;
                    default:
                        return true;
                    }
                });
            }

            {
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

                    KairosStatusEntry.set_placeholder_text(ShowStatusToString(StorageData.GetShowStatus()));
                    KairosStatusEntry.set_text(StorageData.GetStatus());
                    KairosStatusEditBtn.set_sensitive(false);
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
            }

            {
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
                            EGL3_LOG(LogLevel::Error, "Updating kairos avatar encountered an error");
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
                            EGL3_LOG(LogLevel::Error, "Updating kairos background encountered an error");
                        }
                    });
                });
                KairosStatusBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
                    auto Data = (Widgets::AsyncImageKeyed<Json::ShowStatus>*)child->get_child()->get_data("EGL3_ImageBase");
                    if (Data->GetKey() == CurrentUserModel.Get<FriendCurrent>().GetShowStatus()) {
                        return;
                    }

                    StorageData.SetShowStatus(Data->GetKey());
                    KairosStatusEntry.set_placeholder_text(ShowStatusToString(Data->GetKey()));
                    CurrentUserModel.Get<FriendCurrent>().SetShowStatus(Data->GetKey());
                    XmppClient->SetPresence(CurrentUserModel.Get<FriendCurrent>().BuildPresence());
                });

                KairosStatusEntry.signal_changed().connect([this]() {
                    KairosStatusEditBtn.set_sensitive(true);
                });

                KairosStatusEditBtn.signal_clicked().connect([this]() {
                    KairosStatusEditBtn.set_sensitive(false);

                    StorageData.SetStatus(KairosStatusEntry.get_text());
                    CurrentUserModel.Get<FriendCurrent>().SetDisplayStatus(KairosStatusEntry.get_text());
                    XmppClient->SetPresence(CurrentUserModel.Get<FriendCurrent>().BuildPresence());
                });
            }

            {
                CurrentUserModel.Get().SetUpdateCallback([this](const FriendBase& Status) { CurrentUserWidget.Update(); });
                CurrentUserContainer.pack_start(CurrentUserWidget, true, true);
                CurrentUserWidget.SetAsCurrentUser(KairosMenu);
            }

            {
                UpdateUIDispatcher.connect([this]() { UpdateUI(); });
                FriendsRealizeDispatcher.connect([this]() { FriendsRealize(); });
                ResortBoxDispatcher.connect([this]() { ResortBox(); });
            }

            Modules.GetModule<AuthorizationModule>().AuthChanged.connect(sigc::mem_fun(*this, &FriendsModule::OnAuthChanged));

            {
                CurrentUserModel.Get<FriendCurrent>().SetDisplayStatus(StorageData.GetStatus());
                CurrentUserModel.Get<FriendCurrent>().SetShowStatus(StorageData.GetShowStatus());
            }

            {
                KairosStatusBox.foreach([this](Gtk::Widget& Widget) { KairosStatusBox.remove(Widget); });

                static const std::initializer_list<Json::ShowStatus> Statuses{ Json::ShowStatus::Online, Json::ShowStatus::Away, Json::ShowStatus::ExtendedAway, Json::ShowStatus::DoNotDisturb, Json::ShowStatus::Chat };

                StatusWidgets.clear();
                StatusWidgets.reserve(Statuses.size());
                for (auto Status : Statuses) {
                    auto& Widget = StatusWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<Json::ShowStatus>>(Status, "", 48, 48, &Json::ShowStatusToUrl, ImageCache));
                    KairosStatusBox.add(*Widget);
                }

                KairosStatusBox.show_all_children();
            }
        }

    void FriendsModule::OnPresenceUpdate(const std::string& AccountId, Web::Xmpp::Json::Presence&& Presence) {
        if (AccountId != LauncherClient->GetAuthData().AccountId) {
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

    void FriendsModule::OnSystemMessage(Messages::SystemMessage&& NewMessage) {
        if (StorageData.HasFlag<StoredFriendData::AutoDeclineReqs>() && NewMessage.GetAction() == Messages::SystemMessage::ActionType::RequestInbound) {
            AsyncFF.Enqueue([this](auto& AccountId) { LauncherClient->RemoveFriend(AccountId); }, NewMessage.GetAccountId());
            return;
        }

        std::lock_guard Lock(FriendsRealizeMutex);
        FriendsRealizeData.emplace_back(std::move(NewMessage));

        FriendsRealizeDispatcher.emit();
    }

    void FriendsModule::OnAuthChanged(Web::Epic::EpicClientAuthed& FNClient, Web::Epic::EpicClientAuthed& LauncherClient) {
        auto& AuthData = LauncherClient.GetAuthData();
        EGL3_CONDITIONAL_LOG(AuthData.AccountId.has_value(), LogLevel::Critical, "Launcher client does not have an attached account id");

        this->LauncherClient = &LauncherClient;
        CurrentUserModel.Get<FriendCurrent>().SetCurrentUserData(AuthData.AccountId.value(), AuthData.DisplayName.value());
        XmppClient.emplace(
            AuthData.AccountId.value(), AuthData.AccessToken,
            Callbacks{
                [this](const auto& A, auto&& B) { OnPresenceUpdate(A, std::move(B)); },
                [this](auto&& A) { OnSystemMessage(std::move(A)); },
            }
        );
        AddFriendBtn.set_sensitive(true);

        UpdateAsync();
    }

    void FriendsModule::OnFriendAction(Widgets::FriendItemMenu::ClickAction Action, const Friend& FriendData) {
        auto& Friend = FriendData.Get();
        switch (Action)
        {
        case Widgets::FriendItemMenu::ClickAction::CHAT:
            EGL3_LOG(LogLevel::Info, "Chatting button pressed");
            break;
        case Widgets::FriendItemMenu::ClickAction::ACCEPT_REQUEST:
            AsyncFF.Enqueue([this](auto& AccountId) { LauncherClient->AddFriend(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::REMOVE_FRIEND:
        case Widgets::FriendItemMenu::ClickAction::DECLINE_REQUEST:
        case Widgets::FriendItemMenu::ClickAction::CANCEL_REQUEST:
            AsyncFF.Enqueue([this](auto& AccountId) { LauncherClient->RemoveFriend(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::SET_NICKNAME:
            AsyncFF.Enqueue([this](auto& AccountId) { LauncherClient->SetFriendAlias(AccountId, "yoo"); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::BLOCK_USER:
            AsyncFF.Enqueue([this](auto& AccountId) { LauncherClient->BlockUser(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::UNBLOCK_USER:
            AsyncFF.Enqueue([this](auto& AccountId) { LauncherClient->UnblockUser(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::COPY_USER_ID:
            EGL3_LOG(LogLevel::Info, "Copy id button pressed");
            break;
        }
    }

    void FriendsModule::OnOpenViewFriends() {
        ViewFriendsBtn.set_visible(false);
        SwitchStack.set_visible_child(SwitchStackPage0);
    }

    void FriendsModule::OnOpenAddFriendPage() {
        AddFriendStatus.set_text("");

        ViewFriendsBtn.set_visible(true);
        SwitchStack.set_visible_child(SwitchStackPage1);
    }

    void FriendsModule::OnSendFriendRequest() {
        if (!AddFriendSendBtn.get_sensitive()) {
            return;
        }

        if (!LauncherClient) {
            return;
        }

        AddFriendSendBtn.set_sensitive(false);
        FriendRequestTask = std::async(std::launch::async, [this](std::string Text) {
            Utils::EmitRAII Emitter(FriendRequestDispatcher);

            if (Text.empty()) {
                FriendRequestStatus = FriendRequestStatusType::Failure;
                FriendRequestStatusText = "Enter a display name, email address, or an account id";
                return;
            }

            std::array<std::optional<Web::Response<Web::Epic::Responses::GetAccounts::Account>>, 3> Resps;

            const static std::regex IdRegex("[0-9a-f]{32}");
            if (std::regex_match(Text, IdRegex)) {
                if (Text == LauncherClient->GetAuthData().AccountId.value()) {
                    FriendRequestStatus = FriendRequestStatusType::Failure;
                    FriendRequestStatusText = "You can't send a friend request to yourself!";
                    return;
                }

                auto Resp = LauncherClient->GetAccountById(Text);
                if (!Resp.HasError()) {
                    SendFriendRequest(Resp.Get());
                    return;
                }
                Resps[0].emplace(std::move(Resp));
            }

            {
                if (Text == LauncherClient->GetAuthData().DisplayName.value()) {
                    FriendRequestStatus = FriendRequestStatusType::Failure;
                    FriendRequestStatusText = "You can't send a friend request to yourself!";
                    return;
                }

                auto Resp = LauncherClient->GetAccountByDisplayName(Text);
                if (!Resp.HasError()) {
                    SendFriendRequest(Resp.Get());
                    return;
                }
                Resps[2].emplace(std::move(Resp));
            }

            const static std::regex EmailRegex("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
            if (std::regex_match(Text, EmailRegex)) {
                auto Resp = LauncherClient->GetAccountByEmail(Text);
                if (!Resp.HasError()) {
                    if (Resp->Id == LauncherClient->GetAuthData().AccountId.value()) {
                        FriendRequestStatus = FriendRequestStatusType::Failure;
                        FriendRequestStatusText = "You can't send a friend request to yourself!";
                        return;
                    }

                    SendFriendRequest(Resp.Get());
                    return;
                }
                Resps[1].emplace(std::move(Resp));             
            }

            for (auto& Resp : Resps) {
                if (!Resp.has_value()) {
                    continue;
                }

                if (Resp->GetError().GetHttpCode() == 429) {
                    FriendRequestStatus = FriendRequestStatusType::Ratelimited;
                    FriendRequestStatusText = "You've been ratelimited. Try again later?";
                    return;
                }
            }

            FriendRequestStatus = FriendRequestStatusType::Failure;
            FriendRequestStatusText = "Not sure who that is, check what you wrote and try again.";
        }, AddFriendEntry.get_text());
    }

    void FriendsModule::SendFriendRequest(const Web::Epic::Responses::GetAccounts::Account& Account)
    {
        auto Resp = LauncherClient->AddFriend(Account.Id);
        if (!Resp.HasError()) {
            FriendRequestStatus = FriendRequestStatusType::Success;
            FriendRequestStatusText = Utils::Format("Sent %s a friend request!", Account.GetDisplayName().c_str());
        }
        else {
            FriendRequestStatus = FriendRequestStatusType::Failure;
            switch (Utils::Crc32(Resp.GetError().GetErrorCode()))
            {
            case Utils::Crc32("errors.com.epicgames.friends.duplicate_friendship"):
                FriendRequestStatusText = Utils::Format("You're already friends with %s!", Account.GetDisplayName().c_str());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.friend_request_already_sent"):
                FriendRequestStatusText = Utils::Format("You've already sent a friend request to %s!", Account.GetDisplayName().c_str());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.inviter_friendships_limit_exceeded"):
                FriendRequestStatusText = Utils::Format("You have too many friends!", Account.GetDisplayName().c_str());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.invitee_friendships_limit_exceeded"):
                FriendRequestStatusText = Utils::Format("%s has too many friends!", Account.GetDisplayName().c_str());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.incoming_friendships_limit_exceeded"):
                FriendRequestStatusText = Utils::Format("%s has too many friend requests!", Account.GetDisplayName().c_str());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.cannot_friend_due_to_target_settings"):
                FriendRequestStatusText = Utils::Format("%s has disabled friend requests!", Account.GetDisplayName().c_str());
                break;
            default:
                FriendRequestStatusText = Utils::Format("An error occurred: %s", Resp.GetError().GetErrorCode().c_str());
                break;
            }
        }
    }

    void FriendsModule::DisplaySendFriendRequestStatus() {
        AddFriendStatus.get_style_context()->remove_class("friendstatus-success");
        AddFriendStatus.get_style_context()->remove_class("friendstatus-ratelimit");
        AddFriendStatus.get_style_context()->remove_class("friendstatus-failure");
        switch (FriendRequestStatus)
        {
        case FriendRequestStatusType::Success:
            AddFriendStatus.get_style_context()->add_class("friendstatus-success");
            AddFriendEntry.set_text("");
            break;
        case FriendRequestStatusType::Ratelimited:
            AddFriendStatus.get_style_context()->add_class("friendstatus-ratelimit");
            break;
        case FriendRequestStatusType::Failure:
            AddFriendStatus.get_style_context()->add_class("friendstatus-failure");
            break;
        }
        AddFriendStatus.set_text(FriendRequestStatusText);

        AddFriendSendBtn.set_sensitive(true);
    }

    void FriendsModule::UpdateSelection() {
        StorageData.SetFlags(StoredFriendData::OptionFlags(
            (CheckFriendsOffline.get_active() ? (uint8_t)StoredFriendData::ShowOffline : 0) |
            (CheckFriendsOutgoing.get_active() ? (uint8_t)StoredFriendData::ShowOutgoing : 0) |
            (CheckFriendsIncoming.get_active() ? (uint8_t)StoredFriendData::ShowIncoming : 0) |
            (CheckFriendsBlocked.get_active() ? (uint8_t)StoredFriendData::ShowBlocked : 0) |
            (CheckDeclineReqs.get_active() ? (uint8_t)StoredFriendData::AutoDeclineReqs : 0) |
            (CheckProfanity.get_active() ? (uint8_t)StoredFriendData::CensorProfanity : 0)
        ));
        Box.invalidate_filter();
        Box.invalidate_sort();
    }

    void FriendsModule::ResortWidget(Widgets::FriendItem& Widget) {
        std::lock_guard Lock(ResortBoxMutex);
        ResortBoxData.emplace_back(std::ref(Widget));

        ResortBoxDispatcher.emit();
    }

    void FriendsModule::UpdateAsync() {
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
                    AccountIds.emplace_back(LauncherClient->GetAuthData().AccountId.value());
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
                            if (AccountSetting.AccountId != LauncherClient->GetAuthData().AccountId.value()) {
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

                ItemDataError = Web::ErrorData::Status::Success;
            }

            ItemDataGuard.unlock();
            UpdateUIDispatcher.emit();
        });
    }

    void FriendsModule::UpdateUI() {
        std::lock_guard Guard(ItemDataMutex);

        // FriendsResp handling
        {
            Box.foreach([this](Gtk::Widget& Widget) { Box.remove(Widget); });

            FriendsWidgets.clear();
            FriendsWidgets.reserve(FriendsData.size());

            for (auto& Friend : FriendsData) {
                auto& Widget = FriendsWidgets.emplace_back(std::make_unique<Widgets::FriendItem>(*Friend, ImageCache));
                Widget->SetContextMenu(FriendMenu);
                Friend->Get().SetUpdateCallback([Widget = std::ref(*Widget), this](const auto& Status) { Widget.get().Update(); ResortWidget(Widget.get()); });
                Box.add(*Widget);
            }

            Box.invalidate_sort();
            Box.show_all_children();
        }

        // KairosAvatarsResp handling
        {
            KairosAvatarBox.foreach([this](Gtk::Widget& Widget) { KairosAvatarBox.remove(Widget); });

            AvatarsWidgets.clear();
            AvatarsWidgets.reserve(AvatarsData.size());

            for (auto& Avatar : AvatarsData) {
                auto& Widget = AvatarsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Avatar, Json::PresenceKairosProfile::GetDefaultKairosAvatarUrl(), 64, 64, &Json::PresenceKairosProfile::GetKairosAvatarUrl, ImageCache));
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
                auto& Widget = BackgroundsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Background, Json::PresenceKairosProfile::GetDefaultKairosBackgroundUrl(), 64, 64, &Json::PresenceKairosProfile::GetKairosBackgroundUrl, ImageCache));
                KairosBackgroundBox.add(*Widget);
            }

            KairosBackgroundBox.show_all_children();
        }

        UpdateCurrentlyRunning = false;
        UpdateCurrentlyRunning.notify_all();
    }

    void FriendsModule::FriendsRealize() {
        std::lock_guard Guard(FriendsRealizeMutex);

        for (auto& Action : FriendsRealizeData) {
            std::unique_ptr<Friend>* FriendPtr;
            auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&](auto& Friend) {
                return Action.GetAccountId() == Friend->Get().GetAccountId();
            });
            if (FriendItr != FriendsData.end()) {
                FriendPtr = &*FriendItr;
            }
            else {
                FriendPtr = &FriendsData.emplace_back(std::make_unique<Friend>(Friend::ConstructInvalid, Action.GetAccountId(), ""));

                (*FriendPtr)->InitializeAccountSettings(*LauncherClient, AsyncFF);

                auto& Widget = FriendsWidgets.emplace_back(std::make_unique<Widgets::FriendItem>(**FriendPtr, ImageCache));
                Widget->SetContextMenu(FriendMenu);
                (*FriendPtr)->Get().SetUpdateCallback([Widget = std::ref(*Widget), this](const auto& Status) { Widget.get().Update(); ResortWidget(Widget.get()); });
                Box.add(*Widget);
            }

            auto& Friend = **FriendPtr;

            switch (Action.GetAction())
            {
            case Messages::SystemMessage::ActionType::RequestAccept:
                Friend.SetAddFriendship(*LauncherClient, AsyncFF);
                break;
            case Messages::SystemMessage::ActionType::RequestInbound:
                Friend.SetRequestInbound(*LauncherClient, AsyncFF);
                break;
            case Messages::SystemMessage::ActionType::RequestOutbound:
                Friend.SetRequestOutbound(*LauncherClient, AsyncFF);
                break;
            case Messages::SystemMessage::ActionType::Remove:
                Friend.SetRemoveFriendship(*LauncherClient, AsyncFF);
                break;
            case Messages::SystemMessage::ActionType::Block:
                Friend.SetBlocked(*LauncherClient, AsyncFF);
                break;
            case Messages::SystemMessage::ActionType::Unblock:
                Friend.SetUnblocked(*LauncherClient, AsyncFF);
                break;
            case Messages::SystemMessage::ActionType::Update:
                Friend.QueueUpdate(*LauncherClient, AsyncFF);
                break;
            }
        }

        FriendsRealizeData.clear();
    }

    void FriendsModule::ResortBox() {
        std::lock_guard Guard(ResortBoxMutex);

        for (auto& WidgetInfo : ResortBoxData) {
            Gtk::Widget& Widget = WidgetInfo.get();
            ((Gtk::ListBoxRow*)Widget.get_parent())->changed();
        }

        ResortBoxData.clear();
    }
}