#pragma once

#include "../../storage/models/Friend.h"
#include "../../utils/Callback.h"
#include "../../widgets/FriendItemMenu.h"
#include "../../widgets/FriendList.h"
#include "../ModuleList.h"
#include "../ImageCache.h"
#include "Options.h"

#include <future>
#include <gtkmm.h>

namespace EGL3::Modules::Friends {
    class ListModule : public BaseModule {
    public:
        ListModule(ModuleList& Ctx);

        Storage::Models::Friend* GetUser(const std::string& AccountId);

        Storage::Models::FriendCurrent& GetCurrentUser();

        template<class... ArgsT>
        Storage::Models::Friend& AddFriend(ArgsT&&... Args) {
            return *FriendsData.emplace_back(std::forward<ArgsT>(Args)...);
        }

        void DisplayFriend(Storage::Models::Friend& Friend);

        void RefreshFilter();

        void RefreshList();

        Utils::Callback<void(Widgets::FriendItemMenu::ClickAction, Storage::Models::Friend&)> FriendMenuAction;

    private:
        std::weak_ordering CompareFriends(Storage::Models::Friend& A, Storage::Models::Friend& B) const;
        bool FilterFriend(Storage::Models::Friend& Friend) const;

        void SetKairosMenuWindow(Gtk::Window& Window);
        friend class KairosMenuModule;

        ImageCacheModule& ImageCache;
        OptionsModule& Options;

        Widgets::FriendList FriendList;

        Widgets::FriendItemMenu FriendMenu;

        Widgets::FriendList CurrentUserList;
        Storage::Models::Friend CurrentUserModel;

        Gtk::SearchEntry& FilterEntry;

        std::vector<std::unique_ptr<Storage::Models::Friend>> FriendsData;
    };
}