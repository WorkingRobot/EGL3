#include "List.h"

#include "../../utils/StringCompare.h"

namespace EGL3::Modules::Friends {
    using namespace Web::Xmpp;
    using namespace Storage::Models;

    ListModule::ListModule(ModuleList& Ctx) :
        ImageCache(Ctx.GetModule<ImageCacheModule>()),
        Options(Ctx.GetModule<OptionsModule>()),
        FriendList(Ctx.GetWidget<Gtk::TreeView>("FriendsTree"), ImageCache, [this](Storage::Models::Friend& A, Storage::Models::Friend& B) { return CompareFriends(A, B); }, [this](Storage::Models::Friend& Friend) { return FilterFriend(Friend); }),
        CurrentUserContainer(Ctx.GetWidget<Gtk::Box>("FriendsCurrentUserContainer")),
        CurrentUserModel(Friend::ConstructCurrent),
        CurrentUserWidget(CurrentUserModel, ImageCache),
        FilterEntry(Ctx.GetWidget<Gtk::SearchEntry>("FriendsFilterEntry"))
    {
        FriendList.FriendClicked.Set([this](Storage::Models::Friend& Friend, Widgets::FriendList::CellType Cell, int CellX, int CellY, Gdk::Rectangle& PopupRect) {
            FriendMenu.PopupMenu(Friend, FriendList.Get(), PopupRect);
        });

        FriendList.FriendTooltipped.Set([this](Storage::Models::Friend& Friend, Widgets::FriendList::CellType Cell, int, int, const Glib::RefPtr<Gtk::Tooltip>& Tooltip) {
            switch (Cell)
            {
            // TODO: connect this to set the avatar tooltip to the character name instead of show status (with game files?)
            case Widgets::FriendList::CellType::Avatar:
            {
                if (Friend.GetType() == FriendType::NORMAL) {
                    auto& FriendData = Friend.Get<Storage::Models::FriendReal>();
                    Tooltip->set_text(Json::ShowStatusToString(FriendData.GetShowStatus()));
                }
                else {
                    Tooltip->set_text(Json::ShowStatusToString(Json::ShowStatus::Offline));
                }
                return true;
            }
            case Widgets::FriendList::CellType::CenterText:
            {
                if (Friend.GetType() == FriendType::NORMAL) {
                    auto& FriendData = Friend.Get<Storage::Models::FriendReal>();
                    auto& Status = FriendData.GetStatus();
                    if (!Status.empty()) {
                        Tooltip->set_text(FriendData.GetStatus());
                        return true;
                    }
                }
                return false;
            }
            case Widgets::FriendList::CellType::Product:
            {
                if (Friend.GetType() == FriendType::NORMAL) {
                    auto& FriendData = Friend.Get<Storage::Models::FriendReal>();
                    auto Platform = FriendData.GetPlatform();
                    if (!Platform.empty()) {
                        Tooltip->set_text(Json::PresenceStatus::GetPlatformName(Platform));
                        return true;
                    }
                    return true;
                }
                return false;
            }
            default:
                return false;
            }
        });

        FriendMenu.OnAction.Set([this](auto Action, const auto& Friend) { FriendMenuAction(Action, Friend); });

        FilterEntry.signal_changed().connect([this]() { FriendList.Refilter(); FriendList.Resort(); });

        {
            CurrentUserModel.Get().OnUpdate.connect([this]() { CurrentUserWidget.Update(); });
            CurrentUserContainer.pack_start(CurrentUserWidget, true, true);
            // When KairosMenuModule is created, it'll run SetKairosMenuWindow
        }
    }

    Storage::Models::Friend* ListModule::GetUser(const std::string& AccountId)
    {
        auto FriendItr = std::find_if(FriendsData.begin(), FriendsData.end(), [&AccountId](const auto& Friend) {
            return Friend->Get().GetAccountId() == AccountId;
        });
        if (FriendItr != FriendsData.end()) {
            return &**FriendItr;
        }
        return nullptr;
    }

    FriendCurrent& ListModule::GetCurrentUser()
    {
        return CurrentUserModel.Get<FriendCurrent>();
    }

    void ListModule::DisplayFriend(Storage::Models::Friend& FriendContainer)
    {
        FriendList.Add(FriendContainer);
    }

    void ListModule::RefreshFilter()
    {
        FriendList.Refilter();
    }

    void ListModule::RefreshList()
    {
        FriendList.Clear();

        for (auto& Friend : FriendsData) {
            FriendList.Add(*Friend);
        }
    }

    std::weak_ordering ListModule::CompareFriends(Storage::Models::Friend& A, Storage::Models::Friend& B) const
    {
        if (FilterEntry.get_text_length()) [[unlikely]] {
                std::string Text = FilterEntry.get_text();
                auto& FriendA = A.Get();
                auto& FriendB = B.Get();
                auto Comp = Utils::ContainsStringsInsensitive(FriendB.GetDisplayName(), Text) <=> Utils::ContainsStringsInsensitive(FriendA.GetDisplayName(), Text);
                if (std::is_eq(Comp)) {
                    return Utils::ContainsStringsInsensitive(FriendB.GetSecondaryName(), Text) <=> Utils::ContainsStringsInsensitive(FriendA.GetSecondaryName(), Text);
                }
                return Comp;
        }
        else {
            return A <=> B;
        }
    }

    bool ListModule::FilterFriend(Storage::Models::Friend& Data) const
    {
        if (FilterEntry.get_text_length()) {
            std::string Text = FilterEntry.get_text();
            auto& Friend = Data.Get();
            if (Utils::ContainsStringsInsensitive(Friend.GetUsername(), Text) == std::string::npos &&
                Utils::ContainsStringsInsensitive(Friend.GetNickname(), Text) == std::string::npos) {
                return false;
            }
        }

        switch (Data.GetType()) {
        case FriendType::INVALID:
            return false;
        case FriendType::BLOCKED:
            return Options.GetStorageData().HasFlag<StoredFriendData::ShowBlocked>();
        case FriendType::OUTBOUND:
            return Options.GetStorageData().HasFlag<StoredFriendData::ShowOutgoing>();
        case FriendType::INBOUND:
            return Options.GetStorageData().HasFlag<StoredFriendData::ShowIncoming>();
        case FriendType::NORMAL:
            return Data.Get<FriendCurrent>().GetShowStatus() != Json::ShowStatus::Offline || Options.GetStorageData().HasFlag<StoredFriendData::ShowOffline>();

            // TODO: check if i can show a million friends without impacting performance :)
            // If not offline, or less than 500 friends and show offline, or show override
            //return Data.Get<FriendCurrent>().GetShowStatus() != Json::ShowStatus::Offline ||
            //    (FriendsData.size() < 500 && Options.GetStorageData().HasFlag<StoredFriendData::ShowOffline>()) ||
            //    Options.GetStorageData().HasFlag<StoredFriendData::ShowOverride>();
        default:
            return true;
        }
    }

    void ListModule::SetKairosMenuWindow(Gtk::Window& Window)
    {
        CurrentUserWidget.SetAsCurrentUser(Window);
    }
}