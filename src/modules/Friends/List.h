#pragma once

#include "../../storage/models/Friend.h"
#include "../../utils/Callback.h"
#include "../../widgets/CurrentUserItem.h"
#include "../../widgets/FriendItem.h"
#include "../../widgets/FriendItemMenu.h"
#include "../../widgets/CellRendererAvatarStatus.h"
#include "../../widgets/CellRendererCenterText.h"
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

        void ClearFriends();

        template<class... ArgsT>
        Storage::Models::Friend& AddFriend(ArgsT&&... Args) {
            return *FriendsData.emplace_back(std::forward<ArgsT>(Args)...);
        }

        void DisplayFriend(Storage::Models::Friend& Friend);

        void RefreshFilter();

        void RefreshList();

        Utils::Callback<void(Widgets::FriendItemMenu::ClickAction, const Storage::Models::Friend&)> FriendMenuAction;

    private:
        void SetupColumns();

        void UpdateFriendRow(const Gtk::TreeRow& Row);

        void SetKairosMenuWindow(Gtk::Window& Window);
        friend class KairosMenuModule;

        ImageCacheModule& ImageCache;
        OptionsModule& Options;

        Gtk::TreeView& TreeView;

        Glib::RefPtr<Gtk::ListStore> ListStore;
        Glib::RefPtr<Gtk::TreeModelFilter> ListFilter;

        Widgets::CellRendererAvatarStatus<std::string, std::string, EGL3::Web::Xmpp::Json::ShowStatus> AvatarRenderer;
        Widgets::CellRendererCenterText CenterTextRenderer;
        Widgets::CellRendererAvatarStatus<std::string, void, std::string> ProductRenderer;

        struct ModelColumns : public Gtk::TreeModel::ColumnRecord
        {
            ModelColumns()
            {
                add(Data);
                add(KairosAvatar);
                add(KairosBackground);
                add(Status);
                add(DisplayNameMarkup);
                add(Description);
                add(Product);
                add(Platform);
            }

            Gtk::TreeModelColumn<Storage::Models::Friend*> Data;
            Gtk::TreeModelColumn<Glib::ustring> KairosAvatar;
            Gtk::TreeModelColumn<Glib::ustring> KairosBackground;
            Gtk::TreeModelColumn<EGL3::Web::Xmpp::Json::ShowStatus> Status;
            Gtk::TreeModelColumn<Glib::ustring> DisplayNameMarkup;
            Gtk::TreeModelColumn<Glib::ustring> Description;
            Gtk::TreeModelColumn<Glib::ustring> Product;
            Gtk::TreeModelColumn<Glib::ustring> Platform;
        };
        ModelColumns Columns;

        Widgets::FriendItemMenu FriendMenu;

        Gtk::Box& CurrentUserContainer;
        Storage::Models::Friend CurrentUserModel;
        Widgets::CurrentUserItem CurrentUserWidget;

        Gtk::SearchEntry& FilterEntry;

        std::vector<std::unique_ptr<Storage::Models::Friend>> FriendsData;

        std::mutex ResortListMutex;
        std::vector<std::reference_wrapper<Widgets::FriendItem>> ResortListData;
        Glib::Dispatcher ResortListDispatcher;
    };
}