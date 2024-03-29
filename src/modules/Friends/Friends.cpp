#include "Friends.h"

#include "../Login/Auth.h"

#include <array>
#include <regex>

namespace EGL3::Modules::Friends {
    using namespace Web::Xmpp;
    using namespace Storage::Models;

    FriendsModule::FriendsModule(ModuleList& Ctx) :
        Auth(Ctx.GetModule<Login::AuthModule>()),
        Game(Ctx.GetModule<Game::GameModule>()),
        ImageCache(Ctx.GetModule<ImageCacheModule>()),
        AsyncFF(Ctx.GetModule<AsyncFFModule>()),
        SysTray(Ctx.GetModule<SysTrayModule>()),

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
        SwitchStackPage3(Ctx.GetWidget<Gtk::Widget>("FriendsStackPage3")),

        MainStack(Ctx.GetWidget<Gtk::Stack>("MainStack")),
        MainStackFriends(Ctx.GetWidget<Gtk::Widget>("AccountsContainer"))
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

        FriendsChat.RequestDisplayName.Set([this](const auto& AccountId) -> std::string {
            auto FriendPtr = FriendsList.GetUser(AccountId);
            return FriendPtr ? FriendPtr->Get().GetDisplayName() : AccountId;
        });

        FriendsChat.OpenChatPage.Set([this](const auto& AccountId) {
            auto FriendPtr = FriendsList.GetUser(AccountId);
            if (FriendPtr && FriendPtr->GetType() == Storage::Models::FriendType::NORMAL) {
                OnOpenChatPage(*FriendPtr);
            }
        });

        {
            SlotAddFriendSend = AddFriendSendBtn.signal_clicked().connect([this]() { OnSendFriendRequest(); });

            FriendRequestDispatcher.connect([this](AsyncWebRequestStatusType Status, std::string StatusText) {
                AddFriendStatus.get_style_context()->remove_class("friendstatus-success");
                AddFriendStatus.get_style_context()->remove_class("friendstatus-ratelimit");
                AddFriendStatus.get_style_context()->remove_class("friendstatus-failure");
                switch (Status)
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
                AddFriendStatus.set_text(StatusText);

                AddFriendSendBtn.set_sensitive(true);
            });
        }

        {
            SlotSetNickname = SetNicknameBtn.signal_clicked().connect([this]() { OnSetNickname(); });

            SetNicknameDispatcher.connect([this](AsyncWebRequestStatusType Status, std::string StatusText) {
                if (Status != AsyncWebRequestStatusType::Success) {
                    SetNicknameStatusLabel.set_text(StatusText);
                }
                else {
                    OnOpenViewFriends();
                }

                SetNicknameBtn.set_sensitive(true);
            });
        }

            

        {
            UpdateTask.OnStart.Set([this]() { return OnUpdate(); });
            UpdateTask.OnDispatch.Set([this](Web::ErrorData::Status Error) { OnUpdateDispatch(Error); });

            FriendUpdateDispatcher.connect([this](const std::string& AccountId, Web::Epic::Friends::FriendEventType Event) { OnFriendUpdate(AccountId, Event); });
        }

        {
            FriendsList.GetCurrentUser().SetStatusText(Options.GetStatusText());
            FriendsList.GetCurrentUser().SetStatus(Options.GetStatus());
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
                Client.OnPartyInvite.Set([this](const auto& A) { OnPartyInvite(A); });
            });

            AddFriendBtn.set_sensitive(true);

            UpdateTask.Start();
        }
    }

    FriendsModule::~FriendsModule()
    {
        OnOpenViewFriends();
    }

    void FriendsModule::OnPresenceUpdate(const Web::Epic::Friends::Presence& Presence) {
        if (Presence.AccountId != Auth.GetClientLauncher().GetAuthData().AccountId) {
            std::lock_guard Guard(ListMtx);

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
        if (Options.HasFlag<StoredFriendData::AutoDeclineReqs>() && Event == Web::Epic::Friends::FriendEventType::RequestInbound) {
            AsyncFF.Enqueue([this](auto& AccountId) { Auth.GetClientLauncher().RemoveFriend(AccountId); }, AccountId);
            return;
        }

        FriendUpdateDispatcher.emit(AccountId, Event);
    }

    void FriendsModule::OnPartyInvite(const std::string& InviterId)
    {
        auto FriendPtr = FriendsList.GetUser(InviterId);
        if (!FriendPtr) {
            return;
        }

        Utils::ToastTemplate Toast{
            .Type = Utils::ToastType::Text02,
            .TextFields = { "Party Invite", std::format("{} invited you to a party!", FriendPtr->Get().GetDisplayName()) }
        };

        auto State = Game.GetCurrentState();
        std::string Label;
        bool Playable;
        bool Menuable;
        Game.GetStateData(State, Label, Playable, Menuable);

        if (Playable) {
            Toast.Actions.emplace_back(Label);
        }
        else {
            Toast.Type = Utils::ToastType::Text04;
            if (State == Game::GameModule::State::Playing) {
                Toast.TextFields.emplace_back("You're already playing, you can accept the invite in-game.");
            }
            else {
                Toast.TextFields.emplace_back("Launch Fortnite to accept the invite.");
            }
        }
        Toast.Actions.emplace_back("Ignore");
            
        SysTray.ShowToast(Toast, {
            .OnClicked = [this, Playable, State](int ActionIdx) {
                // Only allow the button to work if the state is the same as before (ABA problems aren't really an issue here)
                if (ActionIdx == 0 && Playable && Game.GetCurrentState() == State) {
                    SysTray.SetAppState(SysTrayModule::AppState::Focused);
                    Game.PrimaryButtonClicked();
                }
            }
        });
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
        MainStack.set_visible_child(MainStackFriends);
    }

    void FriendsModule::OnSendFriendRequest() {
        if (!AddFriendSendBtn.get_sensitive()) {
            return;
        }

        AddFriendSendBtn.set_sensitive(false);
        FriendRequestTask = std::async(std::launch::async, [this](std::string Text) {
            if (Text.empty()) {
                FriendRequestDispatcher.emit(AsyncWebRequestStatusType::Failure, "Enter a display name, email address, or an account id");
                return;
            }

            std::array<std::optional<Web::Response<Web::Epic::Responses::GetAccounts::Account>>, 3> Resps;

            const static std::regex IdRegex("[0-9a-f]{32}");
            if (std::regex_match(Text, IdRegex)) {
                if (Text == Auth.GetClientLauncher().GetAuthData().AccountId.value()) {
                    FriendRequestDispatcher.emit(AsyncWebRequestStatusType::Failure, "You can't send a friend request to yourself!");
                    return;
                }

                auto Resp = Auth.GetClientLauncher().GetAccountById(Text);
                if (!Resp.HasError()) {
                    std::string StatusText;
                    FriendRequestDispatcher.emit(SendFriendRequest(Resp.Get(), StatusText), StatusText);
                    return;
                }
                Resps[0].emplace(std::move(Resp));
            }

            {
                if (Text == Auth.GetClientLauncher().GetAuthData().DisplayName.value()) {
                    FriendRequestDispatcher.emit(AsyncWebRequestStatusType::Failure, "You can't send a friend request to yourself!");
                    return;
                }

                auto Resp = Auth.GetClientLauncher().GetAccountByDisplayName(Text);
                if (!Resp.HasError()) {
                    std::string StatusText;
                    FriendRequestDispatcher.emit(SendFriendRequest(Resp.Get(), StatusText), StatusText);
                    return;
                }
                Resps[2].emplace(std::move(Resp));
            }

            const static std::regex EmailRegex("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
            if (std::regex_match(Text, EmailRegex)) {
                auto Resp = Auth.GetClientLauncher().GetAccountByEmail(Text);
                if (!Resp.HasError()) {
                    if (Resp->Id == Auth.GetClientLauncher().GetAuthData().AccountId.value()) {
                        FriendRequestDispatcher.emit(AsyncWebRequestStatusType::Failure, "You can't send a friend request to yourself!");
                        return;
                    }

                    std::string StatusText;
                    FriendRequestDispatcher.emit(SendFriendRequest(Resp.Get(), StatusText), StatusText);
                    return;
                }
                Resps[1].emplace(std::move(Resp));             
            }

            for (auto& Resp : Resps) {
                if (!Resp.has_value()) {
                    continue;
                }

                if (Resp->GetError().GetHttpCode() == 429) {
                    FriendRequestDispatcher.emit(AsyncWebRequestStatusType::Ratelimited, "You've been ratelimited. Try again later?");
                    return;
                }
            }

            FriendRequestDispatcher.emit(AsyncWebRequestStatusType::Failure, "Not sure who that is, check what you wrote and try again.");
        }, AddFriendEntry.get_text());
    }

    FriendsModule::AsyncWebRequestStatusType FriendsModule::SendFriendRequest(const Web::Epic::Responses::GetAccounts::Account& Account, std::string& StatusText)
    {
        auto Resp = Auth.GetClientLauncher().AddFriend(Account.Id);
        if (!Resp.HasError()) {
            StatusText = std::format("Sent {} a friend request!", Account.GetDisplayName().c_str());
            return AsyncWebRequestStatusType::Success;
        }
        else {
            switch (Utils::Crc32(Resp.GetError().GetErrorCode()))
            {
            case Utils::Crc32("errors.com.epicgames.friends.duplicate_friendship"):
                StatusText = std::format("You're already friends with {}!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.friend_request_already_sent"):
                StatusText = std::format("You've already sent a friend request to {}!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.inviter_friendships_limit_exceeded"):
                StatusText = "You have too many friends!";
                break;
            case Utils::Crc32("errors.com.epicgames.friends.invitee_friendships_limit_exceeded"):
                StatusText = std::format("{} has too many friends!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.incoming_friendships_limit_exceeded"):
                StatusText = std::format("{} has too many friend requests!", Account.GetDisplayName());
                break;
            case Utils::Crc32("errors.com.epicgames.friends.cannot_friend_due_to_target_settings"):
                StatusText = std::format("{} has disabled friend requests!", Account.GetDisplayName());
                break;
            default:
                StatusText = std::format("An error occurred: {}", Resp.GetError().GetErrorCode());
                break;
            }
            return AsyncWebRequestStatusType::Failure;
        }
    }

    void FriendsModule::OnSetNickname() {
        if (!SetNicknameBtn.get_sensitive()) {
            return;
        }

        SetNicknameBtn.set_sensitive(false);
        SetNicknameTask = std::async(std::launch::async, [this](std::string AccountId, std::string Text) {
            auto Resp = !Text.empty() ? Auth.GetClientLauncher().SetFriendAlias(AccountId, Text) : Auth.GetClientLauncher().ClearFriendAlias(AccountId);

            if (!Resp.HasError()) {
                SetNicknameDispatcher.emit(AsyncWebRequestStatusType::Success, "");
            }
            else {
                switch (Utils::Crc32(Resp.GetError().GetErrorCode()))
                {
                case Utils::Crc32("errors.com.epicgames.validation.validation_failed"):
                    SetNicknameDispatcher.emit(AsyncWebRequestStatusType::Failure, "The nickname must be 3 to 16 letters, digits, spaces, -, _, . or emoji.");
                    break;
                default:
                    SetNicknameDispatcher.emit(AsyncWebRequestStatusType::Failure, std::format("An error occurred: {}", Resp.GetError().GetErrorCode()));
                    break;
                }
            }
        }, SetNicknameAccountId, SetNicknameEntry.get_text());
    }

    Web::ErrorData::Status FriendsModule::OnUpdate() {
        auto FriendsResp = Auth.GetClientLauncher().GetFriendsSummary();

        if (FriendsResp.HasError()) {
            return FriendsResp.GetErrorCode();
        }
        else {
            std::lock_guard Guard(ListMtx);

            for (auto& Friend : FriendsResp->Friends) {
                FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructNormal, Friend));
            }
            for (auto& Friend : FriendsResp->Incoming) {
                FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructInbound, Friend));
            }
            for (auto& Friend : FriendsResp->Outgoing) {
                FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructOutbound, Friend));
            }
            for (auto& Friend : FriendsResp->Blocklist) {
                FriendsList.AddFriend(std::make_unique<Storage::Models::Friend>(Friend::ConstructBlocked, Friend));
            }

            return Web::ErrorData::Status::Success;
        }
    }

    void FriendsModule::OnUpdateDispatch(Web::ErrorData::Status Error)
    {
        FriendsList.RefreshList();
    }

    void FriendsModule::OnFriendUpdate(const std::string& AccountId, Web::Epic::Friends::FriendEventType Event) {
        auto FriendPtr = FriendsList.GetUser(AccountId);
        if (!FriendPtr) {
            FriendPtr = &FriendsList.AddFriend(std::make_unique<Friend>(Friend::ConstructInvalid, AccountId, ""));

            FriendsList.DisplayFriend(*FriendPtr);
        }

        switch (Event)
        {
        case Web::Epic::Friends::FriendEventType::Added:
            FriendPtr->SetAddFriendship(Auth.GetClientLauncher(), AsyncFF);
            break;
        case Web::Epic::Friends::FriendEventType::RequestInbound:
            FriendPtr->SetRequestInbound(Auth.GetClientLauncher(), AsyncFF);
            SysTray.ShowToast(Utils::ToastTemplate{
                .Type = Utils::ToastType::Text02,
                .TextFields = { "New Friend Request", std::format("{} would like to be your friend", FriendPtr->Get().GetDisplayName()) },
                .Actions = { "Accept", "Decline" }
            }, {
                .OnClicked = [this, FriendPtr](int ActionIdx) {
                    switch (ActionIdx)
                    {
                    case 0:
                        OnFriendAction(Widgets::FriendItemMenu::ClickAction::ACCEPT_REQUEST, *FriendPtr);
                        break;
                    case 1:
                        OnFriendAction(Widgets::FriendItemMenu::ClickAction::DECLINE_REQUEST, *FriendPtr);
                        break;
                    case -1:
                    default:
                        break;
                    }
                }
            });
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
}