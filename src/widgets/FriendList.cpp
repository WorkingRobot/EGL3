#include "FriendList.h"

namespace EGL3::Widgets {
    using namespace Web::Xmpp::Json;

    FriendList::FriendList(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache) :
        ImageCache(ImageCache),
        TreeView(TreeView),
        AvatarRenderer(TreeView, ImageCache, 64, 24, 72),
        ProductRenderer(TreeView, ImageCache, 64, 24, 72)
    {
        ListStore = Gtk::ListStore::create(Columns);
        ListFilter = Gtk::TreeModelFilter::create(ListStore);

        TreeView.set_model(ListFilter);

        SetupColumns();
    }

    void FriendList::Clear()
    {
        ListStore->clear();
    }

    void FriendList::Add(Storage::Models::Friend& Friend)
    {
        auto Itr = ListStore->append();
        auto Ref = Gtk::TreeRowReference(ListStore, ListStore->get_path(Itr));
        auto& Row = *Itr;
        Row[Columns.Data] = &Friend;
        Friend.Get().OnUpdate.connect(
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

    void FriendList::Refilter()
    {
        ListFilter->refilter();
    }

    void FriendList::Resort()
    {
        ListStore->set_sort_column(GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SortType::SORT_DESCENDING);
        ListStore->set_sort_column(GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, Gtk::SortType::SORT_DESCENDING);
    }

    void FriendList::SetupColumns()
    {
        {
            int ColCount = TreeView.append_column("Avatar", AvatarRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->add_attribute(AvatarRenderer.property_foreground(), Columns.KairosAvatar);
                Column->add_attribute(AvatarRenderer.property_background(), Columns.KairosBackground);
                Column->add_attribute(AvatarRenderer.property_icon(), Columns.Status);
            }
            AvatarRenderer.SetDefaultForeground(PresenceKairosProfile::GetDefaultKairosAvatar());
            AvatarRenderer.SetDefaultBackground(PresenceKairosProfile::GetDefaultKairosBackground());
            AvatarRenderer.SetDefaultIcon(ShowStatus::Offline);

            AvatarRenderer.GetForegroundUrl.Set(PresenceKairosProfile::GetKairosAvatarUrl);
            AvatarRenderer.GetBackgroundUrl.Set(PresenceKairosProfile::GetKairosBackgroundUrl);
            AvatarRenderer.GetIconUrl.Set(ShowStatusToUrl);
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

            ProductRenderer.GetForegroundUrl.Set(PresenceStatus::GetProductImageUrl);
            ProductRenderer.GetIconUrl.Set(PresenceStatus::GetPlatformImageUrl);
        }
    }

    void FriendList::UpdateFriendRow(const Gtk::TreeRow& Row)
    {
        auto& FriendContainer = *(EGL3::Storage::Models::Friend*)Row[Columns.Data];
        auto& Friend = FriendContainer.Get();

        Row[Columns.DisplayNameMarkup] = std::format("{} {}", (std::string)Glib::Markup::escape_text(Friend.GetDisplayName()), (std::string)Glib::Markup::escape_text(Friend.GetSecondaryName()));

        Row[Columns.KairosAvatar] = Friend.GetKairosAvatar();
        Row[Columns.KairosBackground] = Friend.GetKairosBackground();

        if (FriendContainer.GetType() == Storage::Models::FriendType::NORMAL || FriendContainer.GetType() == Storage::Models::FriendType::CURRENT) {
            auto& FriendData = FriendContainer.Get<Storage::Models::FriendReal>();

            Row[Columns.Status] = FriendData.GetShowStatus();

            if (FriendData.GetShowStatus() != ShowStatus::Offline) {
                Row[Columns.Product] = (std::string)FriendData.GetProductId();
                Row[Columns.Platform] = (std::string)FriendData.GetPlatform();

                if (FriendData.GetStatus().empty()) {
                    Row[Columns.Description] = ShowStatusToString(FriendData.GetShowStatus());
                }
                else {
                    Row[Columns.Description] = FriendData.GetStatus();
                    // TODO: add this as a tooltip too (somehow...)
                }
            }
            else {
                Row[Columns.Product] = "";
                Row[Columns.Platform] = "";

                Row[Columns.Description] = ShowStatusToString(ShowStatus::Offline);
            }
        }
        else {
            Row[Columns.Status] = ShowStatus::Offline;

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
}
