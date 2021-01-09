#include "List.h"

#include "KairosMenu.h"

namespace EGL3::Modules::Friends {
    using namespace Web::Xmpp;
    using namespace Storage::Models;

    ListModule::ListModule(ModuleList& Modules, const Utils::GladeBuilder& Builder) :
        ImageCache(Modules.GetModule<ImageCacheModule>()),
        Options(Modules.GetModule<OptionsModule>()),
        List(Builder.GetWidget<Gtk::ListBox>("FriendsListBox")),
        CurrentUserContainer(Builder.GetWidget<Gtk::Box>("FriendsCurrentUserContainer")),
        CurrentUserModel(Friend::ConstructCurrent),
        CurrentUserWidget(CurrentUserModel, ImageCache)
    {
        FriendMenu.OnAction.Set([this](auto Action, const auto& Friend) { FriendMenuAction(Action, Friend); });

        {
            List.set_sort_func([](Gtk::ListBoxRow* A, Gtk::ListBoxRow* B) {
                auto APtr = (Widgets::FriendItem*)A->get_child()->get_data("EGL3_FriendBase");
                auto BPtr = (Widgets::FriendItem*)B->get_child()->get_data("EGL3_FriendBase");

                EGL3_CONDITIONAL_LOG(APtr && BPtr, LogLevel::Critical, "Widgets aren't of type FriendItem");

                // Undocumented _Value, but honestly, it's fine
                return (*BPtr <=> *APtr)._Value;
            });

            List.set_filter_func([this](Gtk::ListBoxRow* Row) {
                auto Ptr = (Widgets::FriendItem*)Row->get_child()->get_data("EGL3_FriendBase");

                EGL3_CONDITIONAL_LOG(Ptr, LogLevel::Critical, "Widget isn't of type FriendItem");

                switch (Ptr->GetData().GetType()) {
                case FriendType::INVALID:
                    return false;
                case FriendType::BLOCKED:
                    return Options.GetStorageData().HasFlag<StoredFriendData::ShowBlocked>();
                case FriendType::OUTBOUND:
                    return Options.GetStorageData().HasFlag<StoredFriendData::ShowOutgoing>();
                case FriendType::INBOUND:
                    return Options.GetStorageData().HasFlag<StoredFriendData::ShowIncoming>();
                case FriendType::NORMAL:
                    return Options.GetStorageData().HasFlag<StoredFriendData::ShowOffline>() || Ptr->GetData().Get<FriendCurrent>().GetShowStatus() != Json::ShowStatus::Offline;
                default:
                    return true;
                }
            });
        }

        {
            CurrentUserModel.Get().OnUpdate.connect([this]() { CurrentUserWidget.Update(); });
            CurrentUserContainer.pack_start(CurrentUserWidget, true, true);
            CurrentUserWidget.SetAsCurrentUser(Modules.GetModule<KairosMenuModule>().GetWindow());
        }

        {
            ResortListDispatcher.connect([this]() { ResortList(); });
        }
    }

    void ListModule::ResortEntireList()
    {
        List.invalidate_filter();
        List.invalidate_sort();
    }

    void ListModule::RefreshList()
    {
        // FriendsResp handling
        {
            List.foreach([this](Gtk::Widget& Widget) { List.remove(Widget); });

            FriendsWidgets.clear();
            FriendsWidgets.reserve(FriendsData.size());

            for (auto& Friend : FriendsData) {
                DisplayFriend(*Friend);
            }

            List.show_all_children();
            List.invalidate_sort();
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

    void ListModule::ClearFriends()
    {
        FriendsData.clear();
    }

    void ListModule::DisplayFriend(Storage::Models::Friend& Friend)
    {
        auto& Widget = FriendsWidgets.emplace_back(std::make_unique<Widgets::FriendItem>(Friend, ImageCache));
        Widget->SetContextMenu(FriendMenu);
        Friend.Get().OnUpdate.connect([Widget = std::ref(*Widget), this]() { Widget.get().Update(); ResortWidget(Widget.get()); });
        List.add(*Widget);
    }

    void ListModule::ResortList()
    {
        std::lock_guard Guard(ResortListMutex);

        for (auto& WidgetInfo : ResortListData) {
            Gtk::Widget& Widget = WidgetInfo.get();
            ((Gtk::ListBoxRow*)Widget.get_parent())->changed();
        }

        ResortListData.clear();
    }

    void ListModule::ResortWidget(Widgets::FriendItem& Widget)
    {
        std::lock_guard Lock(ResortListMutex);
        ResortListData.emplace_back(std::ref(Widget));

        ResortListDispatcher.emit();
    }
}