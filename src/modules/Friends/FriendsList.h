#pragma once

#include "../../storage/models/Friend.h"
#include "../../utils/Callback.h"
#include "../../utils/GladeBuilder.h"
#include "../../widgets/CurrentUserItem.h"
#include "../../widgets/FriendItem.h"
#include "../../widgets/FriendItemMenu.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../ImageCache.h"
#include "FriendsOptions.h"

#include <future>
#include <gtkmm.h>

namespace EGL3::Modules {
    class FriendsListModule : public BaseModule {
    public:
        FriendsListModule(ModuleList& Modules, const Utils::GladeBuilder& Builder);

        void ResortEntireList();

        void RefreshList();

        Storage::Models::Friend* GetUser(const std::string& AccountId);

        Storage::Models::FriendCurrent& GetCurrentUser();

        void ClearFriends();

        template<class... ArgsT>
        Storage::Models::Friend& AddFriend(ArgsT&&... Args) {
            return *FriendsData.emplace_back(std::forward<ArgsT>(Args)...);
        }

        void DisplayFriend(Storage::Models::Friend& Friend);

        Utils::Callback<void(Widgets::FriendItemMenu::ClickAction, const Storage::Models::Friend&)> FriendMenuAction;

    private:
        void ResortList();

        void ResortWidget(Widgets::FriendItem& Widget);

        ImageCacheModule& ImageCache;
        FriendsOptionsModule& Options;

        Gtk::ListBox& List;

        Widgets::FriendItemMenu FriendMenu;

        Gtk::Box& CurrentUserContainer;
        Storage::Models::Friend CurrentUserModel;
        Widgets::CurrentUserItem CurrentUserWidget;

        std::vector<std::unique_ptr<Storage::Models::Friend>> FriendsData;
        std::vector<std::unique_ptr<Widgets::FriendItem>> FriendsWidgets;

        std::mutex ResortListMutex;
        std::vector<std::reference_wrapper<Widgets::FriendItem>> ResortListData;
        Glib::Dispatcher ResortListDispatcher;
    };
}