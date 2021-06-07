#include "List.h"

#include "../../utils/StringCompare.h"

namespace EGL3::Modules::Friends {
    using namespace Web::Xmpp;
    using namespace Storage::Models;

    ListModule::ListModule(ModuleList& Ctx) :
        ImageCache(Ctx.GetModule<ImageCacheModule>()),
        Options(Ctx.GetModule<OptionsModule>()),
        TreeView(Ctx.GetWidget<Gtk::TreeView>("FriendsTree")),
        AvatarRenderer(TreeView, ImageCache, 64, 24, 72),
        ProductRenderer(TreeView, ImageCache, 64, 24, 72),
        CurrentUserContainer(Ctx.GetWidget<Gtk::Box>("FriendsCurrentUserContainer")),
        CurrentUserModel(Friend::ConstructCurrent),
        CurrentUserWidget(CurrentUserModel, ImageCache),
        FilterEntry(Ctx.GetWidget<Gtk::SearchEntry>("FriendsFilterEntry"))
    {
        FriendMenu.OnAction.Set([this](auto Action, const auto& Friend) { FriendMenuAction(Action, Friend); });

        ListStore = Gtk::ListStore::create(Columns);
        ListFilter = Gtk::TreeModelFilter::create(ListStore);

        TreeView.set_model(ListFilter);

        SetupColumns();

        ListStore->set_sort_column(Columns.Data, Gtk::SortType::SORT_DESCENDING);
        ListStore->set_sort_func(Columns.Data, [this](const Gtk::TreeModel::iterator& AItr, const Gtk::TreeModel::iterator& BItr) -> int {
            auto& ARow = *AItr;
            auto& BRow = *BItr;
            auto APtr = ARow[Columns.Data];
            auto BPtr = BRow[Columns.Data];

            if (!APtr && !BPtr) {
                return 0;
            }
            else if (!BPtr) {
                return 1;
            }
            else if (!APtr) {
                return -1;
            }
            //EGL3_VERIFY(APtr && BPtr, "Rows do not have a data column attached");

            auto& A = *APtr;
            auto& B = *BPtr;

            std::weak_ordering Comp;

            if (FilterEntry.get_text_length()) [[unlikely]] {
                std::string Text = FilterEntry.get_text();
                auto& FriendA = A.Get();
                auto& FriendB = B.Get();
                Comp = Utils::ContainsStringsInsensitive(FriendA.GetDisplayName(), Text) <=> Utils::ContainsStringsInsensitive(FriendB.GetDisplayName(), Text);
                if (std::is_eq(Comp)) {
                    Comp = Utils::ContainsStringsInsensitive(FriendA.GetSecondaryName(), Text) <=> Utils::ContainsStringsInsensitive(FriendB.GetSecondaryName(), Text);
                }
            }
            else {
                Comp = A <=> B;
            }

            // Undocumented _Value, but honestly, it's fine
            return Comp._Value;
        });

        ListFilter->set_visible_func([this](const Gtk::TreeModel::const_iterator& Itr) {
            auto& Row = *Itr;
            auto Ptr = (Friend*)Row[Columns.Data];

            if (!Ptr) {
                return true;
            }
            //EGL3_VERIFY(Ptr, "Row does not have a data column attached");

            auto& Data = *Ptr;

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
        });

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

    void ListModule::ClearFriends()
    {
        FriendsData.clear();
    }

    void ListModule::DisplayFriend(Storage::Models::Friend& FriendContainer)
    {
        auto Itr = ListStore->append();
        auto Ref = Gtk::TreeRowReference(ListStore, ListStore->get_path(Itr));
        auto& Row = *Itr;
        Row[Columns.Data] = &FriendContainer;
        FriendContainer.Get().OnUpdate.connect(
            [this, Ref = std::move(Ref)]() {
                Glib::signal_idle().connect_once(
                    [this, &Ref]() {
                        UpdateFriendRow(*ListStore->get_iter(Ref.get_path()));
                    }
                );
            }
        );
        UpdateFriendRow(Row);
    }

    void ListModule::RefreshFilter()
    {
        ListFilter->refilter();
    }

    void ListModule::RefreshList()
    {
        ListStore->clear();

        for (auto& Friend : FriendsData) {
            DisplayFriend(*Friend);
        }
    }

    void ListModule::SetupColumns()
    {
        {
            int ColCount = TreeView.append_column("Avatar", AvatarRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->add_attribute(AvatarRenderer.property_foreground(), Columns.KairosAvatar);
                Column->add_attribute(AvatarRenderer.property_background(), Columns.KairosBackground);
                Column->add_attribute(AvatarRenderer.property_icon(), Columns.Status);
            }
            AvatarRenderer.SetDefaultForeground(Json::PresenceKairosProfile::GetDefaultKairosAvatar());
            AvatarRenderer.SetDefaultBackground(Json::PresenceKairosProfile::GetDefaultKairosBackground());
            AvatarRenderer.SetDefaultIcon(Json::ShowStatus::Offline);

            AvatarRenderer.GetForegroundUrl.Set(Json::PresenceKairosProfile::GetKairosAvatarUrl);
            AvatarRenderer.GetBackgroundUrl.Set(Json::PresenceKairosProfile::GetKairosBackgroundUrl);
            AvatarRenderer.GetIconUrl.Set(Json::ShowStatusToUrl);
        }

        {
            int ColCount = TreeView.append_column("CenterText", CenterTextRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->set_expand(true);

                Column->add_attribute(CenterTextRenderer.property_name(), Columns.DisplayNameMarkup);
                Column->add_attribute(CenterTextRenderer.property_status(), Columns.Description);
            }

            CenterTextRenderer.GetRendererB().property_ellipsize() = Pango::EllipsizeMode::ELLIPSIZE_END;
        }

        {
            int ColCount = TreeView.append_column("Product", ProductRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->add_attribute(ProductRenderer.property_foreground(), Columns.Product);
                Column->add_attribute(ProductRenderer.property_icon(), Columns.Platform);
            }
            ProductRenderer.SetDefaultForeground("default");
            ProductRenderer.SetDefaultIcon("OTHER");

            ProductRenderer.GetForegroundUrl.Set(Json::PresenceStatus::GetProductImageUrl);
            ProductRenderer.GetIconUrl.Set(Json::PresenceStatus::GetPlatformImageUrl);
        }
    }

    void ListModule::UpdateFriendRow(const Gtk::TreeRow& Row)
    {
        auto& FriendContainer = *(EGL3::Storage::Models::Friend*)Row[Columns.Data];
        auto& Friend = FriendContainer.Get();

        Row[Columns.DisplayNameMarkup] = std::format("{} {}", (std::string)Glib::Markup::escape_text(Friend.GetDisplayName()), (std::string)Glib::Markup::escape_text(Friend.GetSecondaryName()));

        Row[Columns.KairosAvatar] = Friend.GetKairosAvatar();
        Row[Columns.KairosBackground] = Friend.GetKairosBackground();

        if (FriendContainer.GetType() == Storage::Models::FriendType::NORMAL || FriendContainer.GetType() == Storage::Models::FriendType::CURRENT) {
            auto& FriendData = FriendContainer.Get<Storage::Models::FriendReal>();

            Row[Columns.Status] = FriendData.GetShowStatus();

            if (FriendData.GetShowStatus() != Json::ShowStatus::Offline) {
                Row[Columns.Product] = (std::string)FriendData.GetProductId();
                Row[Columns.Platform] = (std::string)FriendData.GetPlatform();

                if (FriendData.GetStatus().empty()) {
                    Row[Columns.Description] = Json::ShowStatusToString(FriendData.GetShowStatus());
                }
                else {
                    Row[Columns.Description] = FriendData.GetStatus();
                    // TODO: add this as a tooltip too (somehow...)
                }
            }
            else {
                Row[Columns.Product] = "";
                Row[Columns.Platform] = "";

                Row[Columns.Description] = Json::ShowStatusToString(Json::ShowStatus::Offline);
            }
        }
        else {
            Row[Columns.Status] = Json::ShowStatus::Offline;

            switch (FriendContainer.GetType())
            {
            case Storage::Models::FriendType::INBOUND:
                Row[Columns.Description] = "Incoming Friend Request";
                break;
            case Storage::Models::FriendType::OUTBOUND:
                Row[Columns.Description] = "Outgoing Friend Request";
                break;
            case Storage::Models::FriendType::BLOCKED:
                Row[Columns.Description] = "Blocked";
                break;
            default:
                Row[Columns.Description] = "Unknown User";
                break;
            }
        }
    }

    void ListModule::SetKairosMenuWindow(Gtk::Window& Window)
    {
        CurrentUserWidget.SetAsCurrentUser(Window);
    }
}