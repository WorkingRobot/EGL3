#include "Friends.h"

#include "../../utils/EmitRAII.h"
#include "../Login/Auth.h"

#include <array>
#include <regex>

namespace EGL3::Modules::Friends {
    using namespace Web::Xmpp;
    using namespace Storage::Models;

    FriendsModule::FriendsModule(ModuleList& Ctx) :
            Auth(Ctx.GetModule<Login::AuthModule>()),
            ImageCache(Ctx.GetModule<ImageCacheModule>()),
            AsyncFF(Ctx.GetModule<AsyncFFModule>()),

            Options(Ctx.GetModule<OptionsModule>()),
            KairosMenu(Ctx.GetModule<KairosMenuModule>()),
            FriendsList(Ctx.GetModule<ListModule>()),
            FriendsChat(Ctx.GetModule<ChatModule>()),

            ViewFriendsBtn(Ctx.GetWidget<Gtk::Button>("FriendViewFriendsBtn")),
            AddFriendBtn(Ctx.GetWidget<Gtk::Button>("FriendsOpenSendRequestBtn")),

            AddFriendSendBtn(Ctx.GetWidget<Gtk::Button>("FriendsSendRequestBtn")),
            AddFriendEntry(Ctx.GetWidget<Gtk::Entry>("FriendsSendRequestEntry")),
            AddFriendStatus(Ctx.GetWidget<Gtk::Label>("FriendsSendRequestStatus")),

            SetNicknameLabel(Ctx.GetWidget<Gtk::Label>("FriendsSetNicknameLabel")),
            SetNicknameBtn(Ctx.GetWidget<Gtk::Button>("FriendsSetNicknameBtn")),
            SetNicknameEntry(Ctx.GetWidget<Gtk::Entry>("FriendsSetNicknameEntry")),
            SetNicknameStatusLabel(Ctx.GetWidget<Gtk::Label>("FriendsSetNicknameStatus")),

            SwitchStack(Ctx.GetWidget<Gtk::Stack>("FriendsStack")),
            SwitchStackPage0(Ctx.GetWidget<Gtk::Widget>("FriendsStackPage0")),
            SwitchStackPage1(Ctx.GetWidget<Gtk::Widget>("FriendsStackPage1")),
            SwitchStackPage2(Ctx.GetWidget<Gtk::Widget>("FriendsStackPage2")),
            SwitchStackPage3(Ctx.GetWidget<Gtk::Widget>("FriendsStackPage3"))
    {
        {
            SlotViewFriends = ViewFriendsBtn.signal_clicked().connect([this]() { OnOpenViewFriends(); });

            SlotAddFriend = AddFriendBtn.signal_clicked().connect([this]() { OnOpenAddFriendPage(); });

            Options.OnUpdate.Set([this]() { FriendsList.RefreshFilter(); });
        }

        KairosMenu.OnUpdatePresence.Set([this]() {
            if (FriendsClient.has_value()) {
                FriendsClient->SetPresence(FriendsList.GetCurrentUser().BuildPresence());
            }
        });

        FriendsList.FriendMenuAction.Set([this](auto Action, auto& Friend) { OnFriendAction(Action, Friend); });

        FriendsChat.SendChatMessage.Set([this](const auto& AccountId, const auto& Message) {
            EGL3_VERIFY(FriendsClient.has_value(), "Didn't send user message. Friends client isn't created yet");
            FriendsClient->SendChat(AccountId, Message);
        });

        {
            SlotAddFriendSend = AddFriendSendBtn.signal_clicked().connect([this]() { OnSendFriendRequest(); });

            FriendRequestDispatcher.connect([this]() { DisplaySendFriendRequestStatus(); });
        }

        {
            SlotSetNickname = SetNicknameBtn.signal_clicked().connect([this]() { OnSetNickname(); });

            SetNicknameDispatcher.connect([this]() { DisplaySetNicknameStatus(); });
        }

            

        {
            UpdateUIDispatcher.connect([this]() { UpdateUI(); });
            FriendUpdateDispatcher.connect([this]() { FriendsUpdate(); });
        }

        {
            FriendsList.GetCurrentUser().SetStatusText(Options.GetStorageData().GetStatusText());
            FriendsList.GetCurrentUser().SetStatus(Options.GetStorageData().GetStatus());
        }

        {
            auto& AuthData = Auth.GetClientLauncher().GetAuthData();
            EGL3_VERIFY(AuthData.AccountId.has_value(), "Launcher client does not have an attached account id");

            FriendsList.GetCurrentUser().SetCurrentUserData(AuthData.AccountId.value(), AuthData.DisplayName.value());

            AsyncFF.Enqueue([this]() {
                auto& Client = FriendsClient.emplace(Auth.GetClientLauncher(), Auth.GetClientFortnite(), FriendsList.GetCurrentUser().BuildPresence());
                Client.OnPresenceUpdate.Set([this](const auto& A) { OnPresenceUpdate(A); });
                Client.OnChatReceived.Set([this](const auto& A, const auto& B) { OnChatReceived(A, B); });
                Client.OnFriendEvent.Set([this](const auto& A, auto B) { OnFriendEvent(A, B); });
            });

            AddFriendBtn.set_sensitive(true);

            UpdateAsync();
        }
    }

    FriendsModule::~FriendsModule()
    {
        OnOpenViewFriends();
    }

    void FriendsModule::OnPresenceUpdate(const Web::Epic::Friends::Presence& Presence) {
        if (Presence.AccountId != Auth.GetClientLauncher().GetAuthData().AccountId) {
            std::lock_guard Guard(ItemDataMutex);

            auto Friend = FriendsList.GetUser(Presence.AccountId);
            if (Friend && Friend->GetType() == FriendType::NORMAL) {
                Friend->Get<FriendReal>().UpdatePresence(Presence);
            }
        }
        else {
            FriendsList.GetCurrentUser().UpdatePresence(Presence);
        }
    }

    void FriendsModule::OnChatReceived(const std::string& AccountId, const std::string& Message)
    {
        FriendsChat.RecieveChatMessage(AccountId, Message);
    }

    void FriendsModule::OnFriendEvent(const std::string& AccountId, Web::Epic::Friends::FriendEventType Event) {
        if (Options.GetStorageData().HasFlag<StoredFriendData::AutoDeclineReqs>() && Event == Web::Epic::Friends::FriendEventType::RequestInbound) {
            AsyncFF.Enqueue([this](auto& AccountId) { Auth.GetClientLauncher().RemoveFriend(AccountId); }, AccountId);
            return;
        }

        std::lock_guard Guard(FriendUpdateMutex);
        FriendUpdateData.emplace_back(AccountId, Event);

        FriendUpdateDispatcher.emit();
    }

    void FriendsModule::OnFriendAction(Widgets::FriendItemMenu::ClickAction Action, Friend& FriendData) {
        auto& Friend = FriendData.Get();
        switch (Action)
        {
        case Widgets::FriendItemMenu::ClickAction::CHAT:
            OnOpenChatPage(FriendData);
            break;
        case Widgets::FriendItemMenu::ClickAction::ACCEPT_REQUEST:
            AsyncFF.Enqueue([this](auto& AccountId) { Auth.GetClientLauncher().AddFriend(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::REMOVE_FRIEND:
        case Widgets::FriendItemMenu::ClickAction::DECLINE_REQUEST:
        case Widgets::FriendItemMenu::ClickAction::CANCEL_REQUEST:
            AsyncFF.Enqueue([this](auto& AccountId) { Auth.GetClientLauncher().RemoveFriend(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::SET_NICKNAME:
            OnOpenSetNicknamePage(FriendData);
            break;
        case Widgets::FriendItemMenu::ClickAction::BLOCK_USER:
            AsyncFF.Enqueue([this](auto& AccountId) { Auth.GetClientLauncher().BlockUser(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::UNBLOCK_USER:
            AsyncFF.Enqueue([this](auto& AccountId) { Auth.GetClientLauncher().UnblockUser(AccountId); }, Friend.GetAccountId());
            break;
        case Widgets::FriendItemMenu::ClickAction::COPY_USER_ID:
            Gtk::Clipboard::get()->set_text(Friend.GetAccountId());
            break;
        }
    }

    void FriendsModule::OnOpenViewFriends() {
        FriendsChat.ClearUser();

        ViewFriendsBtn.set_visible(false);
        SwitchStack.set_visible_child(SwitchStackPage0);
    }

    void FriendsModule::OnOpenAddFriendPage() {
        AddFriendStatus.set_text("");

        ViewFriendsBtn.set_visible(true);
        SwitchStack.set_visible_child(SwitchStackPage1);
    }

    void FriendsModule::OnOpenSetNicknamePage(const Friend& FriendData) {
        SetNicknameAccountId = FriendData.Get().GetAccountId();
        SetNicknameLabel.set_text(std::format("Set Nickname for {}", FriendData.Get().GetDisplayName()));
        SetNicknameEntry.set_placeholder_text(FriendData.Get().GetUsername());
        SetNicknameEntry.set_text(FriendData.Get().GetNickname());
        SetNicknameStatusLabel.set_text("");

        ViewFriendsBtn.set_visible(true);
        SwitchStack.set_visible_child(SwitchStackPage3);
    }

    void FriendsModule::OnOpenChatPage(Friend& FriendData)
    {
        FriendsChat.SetUser(FriendData);

        ViewFriendsBtn.set_visible(true);
        SwitchStack.set_visible_child(SwitchStackPage2);
    }

    void FriendsModule::OnSendFriendRequest() {
        if (!AddFriendSendBtn.get_sensitive()) {
            return;
        }

        AddFriendSendBtn.set_sensitive(false);
        FriendRequestTask = std::async(std::launch::async, [this](std::string Text) {
            Utils::EmitRAII Emitter(FriendRequestDispatcher);

            if (Text.empty()) {
                FriendRequestStatus = AsyncWebRequestStatusType::Failure;
                FriendRequestStatusText = "Enter a display name, email address, or an account id";
                return;
            }

            std::array<std::optional<Web::Response<Web::Epic::Responses::GetAccounts::Account>>, 3> Resps;

            const static std::regex IdRegex("[0-9a-f]{32}");
            if (std::regex_match(Text, IdRegex)) {
                if (Text == Auth.GetClientLauncher().GetAuthData().AccountId.value()) {
                    FriendRequestStatus = AsyncWebRequestStatusType::Failure;
                    FriendRequestStatusText = "You can't send a friend request to yourself!";
                    return;
                }

                auto Resp = Auth.GetClientLauncher().GetAccountById(Text);
                if (!Resp.HasError()) {
                    SendFriendRequest(Resp.Get());
                    return;
                }
                Resps[0].emplace(std::move(Resp));
            }

            {
                if (Text == Auth.GetClientLauncher().GetAuthData().DisplayName.value()) {
                    FriendRequestStatus = AsyncWebRequestStatusType::Failure;
                    FriendRequestStatusText = "You can't send a friend request to yourself!";
                    return;
                }

                auto Resp = Auth.GetClientLauncher().GetAccountByDisplayName(Text);
                if (!Resp.HasError()) {
                    SendFriendRequest(Resp.Get());
                    return;
                }
                Resps[2].emplace(std::move(Resp));
            }

            const static std::regex EmailRegex("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
            if (std::regex_match(Text, EmailRegex)) {
                auto Resp = Auth.GetClientLauncher().GetAccountByEmail(Text);
                if (!Resp.HasError()) {
                    if (Resp->Id == Auth.GetClientLauncher().GetAuthData().AccountId.value()) {
                        FriendRequestStatus = AsyncWebRequestStatusType::Failure;
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
                    FriendRequestStatus = AsyncWebRequestStatusType::Ratelimited;
                    FriendRequestStatusText = "You've been ratelimited. Try again later?";
                    return;
                }
            }

            FriendRequestStatus = AsyncWebRequestStatusType::Failure;
            FriendRequestStatusText = "Not sure who that is, check what you wrote and try again.";
        }, AddFriendEntry.get_text());
    }

    void FriendsModule::SendFriendRequest(const Web::Epic::Responses::GetAccounts::Account& Account)
    {
        auto Resp = Auth.GetClientLauncher().AddFriend(Account.Id);
        if (!Resp.HasError()) {
            FriendRequestStatus = AsyncWebRequestStatusType::Success;
            FriendRequestStatusText = std::format("Sent {} a friend request!", Account.GetDisplayName().c_str());
        }
        else {
            FriendRequestStatus = AsyncWebRequestStatusType::Failure;
            switch (Utils::Crc32(Resp.GetError().GetErrorCode()))
            {
            case Utils::Crc32("errors.com.epicgames.friends.duplicate_friendship"):
                FriendRequestStatusText = std::format("You're already friends with {}!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.friend_request_already_sent"):
                FriendRequestStatusText = std::format("You've already sent a friend request to {}!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.inviter_friendships_limit_exceeded"):
                FriendRequestStatusText = "You have too many friends!";
                break;
            case Utils::Crc32("errors.com.epicgames.friends.invitee_friendships_limit_exceeded"):
                FriendRequestStatusText = std::format("{} has too many friends!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.incoming_friendships_limit_exceeded"):
                FriendRequestStatusText = std::format("{} has too many friend requests!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.cannot_friend_due_to_target_settings"):
                FriendRequestStatusText = std::format("{} has disabled friend requests!", Account.GetDisplayName());
                break;
            default:
                FriendRequestStatusText = std::format("An error occurred: {}", Resp.GetError().GetErrorCode());
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
        case AsyncWebRequestStatusType::Success:
            AddFriendStatus.get_style_context()->add_class("friendstatus-success");
            AddFriendEntry.set_text("");
            break;
        case AsyncWebRequestStatusType::Ratelimited:
            AddFriendStatus.get_style_context()->add_class("friendstatus-ratelimit");
            break;
        case AsyncWebRequestStatusType::Failure:
            AddFriendStatus.get_style_context()->add_class("friendstatus-failure");
            break;
        }
        AddFriendStatus.set_text(FriendRequestStatusText);

        AddFriendSendBtn.set_sensitive(true);
    }

    void FriendsModule::OnSetNickname() {
        if (!SetNicknameBtn.get_sensitive()) {
            return;
        }

        SetNicknameBtn.set_sensitive(false);
        SetNicknameTask = std::async(std::launch::async, [this](std::string AccountId, std::string Text) {
            Utils::EmitRAII Emitter(SetNicknameDispatcher);

            auto Resp = !Text.empty() ? Auth.GetClientLauncher().SetFriendAlias(AccountId, Text) : Auth.GetClientLauncher().ClearFriendAlias(AccountId);

            if (!Resp.HasError()) {
                SetNicknameStatus = AsyncWebRequestStatusType::Success;
            }
            else {
                SetNicknameStatus = AsyncWebRequestStatusType::Failure;
                switch (Utils::Crc32(Resp.GetError().GetErrorCode()))
                {
                case Utils::Crc32("errors.com.epicgames.validation.validation_failed"):
                    SetNicknameStatusText = "The nickname must be 3 to 16 letters, digits, spaces, -, _, . or emoji.";
                    break;
                default:
                    SetNicknameStatusText = std::format("An error occurred: {}", Resp.GetError().GetErrorCode());
                    break;
                }
            }
        }, SetNicknameAccountId, SetNicknameEntry.get_text());
    }

    void FriendsModule::DisplaySetNicknameStatus() {
        if (SetNicknameStatus != AsyncWebRequestStatusType::Success) {
            SetNicknameStatusLabel.set_text(SetNicknameStatusText);
        }
        else {
            OnOpenViewFriends();
        }

        SetNicknameBtn.set_sensitive(true);
    }

    void FriendsModule::UpdateAsync() {
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

            auto FriendsResp = Auth.GetClientLauncher().GetFriendsSummary();
            auto KairosAvatarsResp = Auth.GetClientLauncher().GetAvailableSettingValues("avatar");
            auto KairosBackgroundsResp = Auth.GetClientLauncher().GetAvailableSettingValues("avatarBackground");

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
                    // Chunking account ids to send, maximum 100 ids per request
                    std::vector<std::string> AccountIds;
                    AccountIds.reserve(FriendsResp->Friends.size() + 1);
                    // We've already asserted that LauncherClient has an account id
                    AccountIds.emplace_back(Auth.GetClientLauncher().GetAuthData().AccountId.value());
                    for (auto& Friend : FriendsResp->Friends) {
                        FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructNormal, Friend));
                        AccountIds.emplace_back(Friend.AccountId);
                    }
                    for (auto& Friend : FriendsResp->Incoming) {
                        FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructInbound, Friend));
                        AccountIds.emplace_back(Friend.AccountId);
                    }
                    for (auto& Friend : FriendsResp->Outgoing) {
                        FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructOutbound, Friend));
                        AccountIds.emplace_back(Friend.AccountId);
                    }
                    for (auto& Friend : FriendsResp->Blocklist) {
                        FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructBlocked, Friend));
                        AccountIds.emplace_back(Friend.AccountId);
                    }

                    std::vector<std::string> SendingAccountIds;
                    // The maximum is actually 100, but Fortnite is nice and uses 50 instead
                    SendingAccountIds.reserve(std::min<size_t>(50, AccountIds.size()));
                    size_t SentAccountIds = 0;
                    while (AccountIds.size() != SentAccountIds) {
                        auto AccountAmt = std::min<size_t>(50, AccountIds.size() - SentAccountIds);
                        std::move(AccountIds.begin() + SentAccountIds, AccountIds.begin() + SentAccountIds + AccountAmt, std::back_inserter(SendingAccountIds));

                        auto FriendsKairosResp = Auth.GetClientLauncher().GetSettingsForAccounts(SendingAccountIds, { "avatar", "avatarBackground" });

                        SendingAccountIds.clear();
                        SentAccountIds += AccountAmt;
                        if (FriendsKairosResp.HasError()) {
                            ItemDataError = FriendsKairosResp.GetErrorCode();
                            break;
                        }

                        for (auto& AccountSetting : FriendsKairosResp->Values) {
                            if (AccountSetting.AccountId != Auth.GetClientLauncher().GetAuthData().AccountId.value()) {
                                auto Friend = FriendsList.GetUser(AccountSetting.AccountId);
                                if (Friend) {
                                    Friend->Get().UpdateAccountSetting(AccountSetting);
                                }
                            }
                            else {
                                FriendsList.GetCurrentUser().UpdateAccountSetting(AccountSetting);
                            }
                        }
                    }
                }

                // KairosAvatarsResp and KairosBackgroundsResp handling
                KairosMenu.SetAvailableSettings(std::move(KairosAvatarsResp->Values), std::move(KairosBackgroundsResp->Values));

                ItemDataError = Web::ErrorData::Status::Success;
            }

            ItemDataGuard.unlock();
            UpdateUIDispatcher.emit();
        });
    }

    void FriendsModule::UpdateUI() {
        std::lock_guard Guard(ItemDataMutex);

        FriendsList.RefreshList();

        KairosMenu.UpdateAvailableSettings();

        UpdateCurrentlyRunning = false;
        UpdateCurrentlyRunning.notify_all();
    }

    void FriendsModule::FriendsUpdate() {
        std::lock_guard Guard(FriendUpdateMutex);

        for (auto& [AccountId, Event] : FriendUpdateData) {
            auto FriendPtr = FriendsList.GetUser(AccountId);
            if (!FriendPtr) {
                FriendPtr = &FriendsList.AddFriend(std::make_unique<Friend>(Friend::ConstructInvalid, AccountId, ""));

                FriendPtr->InitializeAccountSettings(Auth.GetClientLauncher(), AsyncFF);

                FriendsList.DisplayFriend(*FriendPtr);
            }

            switch (Event)
            {
            case Web::Epic::Friends::FriendEventType::Added:
                FriendPtr->SetAddFriendship(Auth.GetClientLauncher(), AsyncFF);
                break;
            case Web::Epic::Friends::FriendEventType::RequestInbound:
                FriendPtr->SetRequestInbound(Auth.GetClientLauncher(), AsyncFF);
                break;
            case Web::Epic::Friends::FriendEventType::RequestOutbound:
                FriendPtr->SetRequestOutbound(Auth.GetClientLauncher(), AsyncFF);
                break;
            case Web::Epic::Friends::FriendEventType::Removed:
                FriendPtr->SetRemoveFriendship(Auth.GetClientLauncher(), AsyncFF);
                break;
            case Web::Epic::Friends::FriendEventType::Blocked:
                FriendPtr->SetBlocked(Auth.GetClientLauncher(), AsyncFF);
                break;
            case Web::Epic::Friends::FriendEventType::Unblocked:
                FriendPtr->SetUnblocked(Auth.GetClientLauncher(), AsyncFF);
                break;
            case Web::Epic::Friends::FriendEventType::Updated:
                FriendPtr->QueueUpdate(Auth.GetClientLauncher(), AsyncFF);
                break;
            }
        }

        FriendUpdateData.clear();
    }
}