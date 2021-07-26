#include "FriendList.h"

#include "../modules/Friends/KairosMenu.h"

namespace EGL3::Widgets {
    FriendList::FriendList(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache) :
        ImageCache(ImageCache),
        TreeView(TreeView),
        AvatarRenderer(TreeView, ImageCache, 64, 24, 72),
        ProductRenderer(TreeView, ImageCache, 64, 24, 72),
        FriendTooltipped([](Storage::Models::Friend&, CellType, int, int, const Glib::RefPtr<Gtk::Tooltip>&) { return false; })
    {
        ListStore = Gtk::ListStore::create(Columns);
        ListFilter = Gtk::TreeModelFilter::create(ListStore);

        TreeView.set_model(ListFilter);
        TreeView.set_headers_visible(false);
        TreeView.set_fixed_height_mode(true);
        SlotTooltip = TreeView.signal_query_tooltip().connect([this](int X, int Y, bool KeyboardTooltip, const Glib::RefPtr<Gtk::Tooltip>& Tooltip) {
            Gtk::TreeModel::Path Path;
            Gtk::TreeViewColumn* Column;
            int CellX = 0, CellY = 0;
            if (KeyboardTooltip) {
                this->TreeView.get_cursor(Path, Column);
                if (Path.empty()) {
                    return false;
                }
            }
            else {
                int BinX, BinY;
                this->TreeView.convert_widget_to_bin_window_coords(X, Y, BinX, BinY);
                this->TreeView.is_blank_at_pos(BinX, BinY, Path, Column, CellX, CellY);
                if (Path.empty()) {
                    return false;
                }
            }

            auto Itr = ListFilter->get_iter(Path);
            if (!Itr) {
                return false;
            }

            auto& Row = *Itr;
            auto FriendPtr = (Storage::Models::Friend*)Row[Columns.Data];
            if (FriendPtr) {
                auto Renderer = Column->get_first_cell();
                if (FriendTooltipped(*FriendPtr, GetCellType(Renderer), CellX, CellY, Tooltip)) {
                    this->TreeView.set_tooltip_cell(Tooltip, &Path, Column, Renderer);
                    return true;
                }
            }
            return false;
        });
        TreeView.set_has_tooltip(true);
        SlotClick = TreeView.signal_button_release_event().connect([this](GdkEventButton* Event) {
            Gtk::TreeModel::Path Path;
            Gtk::TreeViewColumn* Column;
            int CellX = 0, CellY = 0;

            int BinX, BinY;
            this->TreeView.convert_widget_to_bin_window_coords(Event->x, Event->y, BinX, BinY);
            this->TreeView.is_blank_at_pos(BinX, BinY, Path, Column, CellX, CellY);
            if (Path.empty()) {
                return false;
            }

            auto Itr = ListFilter->get_iter(Path);
            if (!Itr) {
                return false;
            }

            auto& Row = *Itr;
            auto FriendPtr = (Storage::Models::Friend*)Row[Columns.Data];
            if (FriendPtr) {
                Gdk::Rectangle Rect;
                this->TreeView.get_background_area(Path, *this->TreeView.get_column(0), Rect);
                FriendClicked(*FriendPtr, GetCellType(Column->get_first_cell()), CellX, CellY, Rect);
            }
            return false;
        });

        SetupColumns();
    }

    FriendList::~FriendList()
    {
        ListStore->clear();

        TreeView.remove_all_columns();
        TreeView.unset_model();
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
        Row[Columns.UpdateSlot] = Friend.Get().OnUpdate.connect(
            [this, ListStoreRef = Glib::WeakRef<Gtk::ListStore>(ListStore), Ref = std::move(Ref)]() {
                Glib::signal_idle().connect_once(
                    [this, ListStoreRef, Ref]() {
                        auto ListStore = ListStoreRef.get();
                        if (Ref.is_valid() && ListStoreRef && ListStore) {
                            UpdateFriendRow(*ListStore->get_iter(Ref.get_path()));
                        }
                    },
                    Glib::PRIORITY_HIGH_IDLE
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
        ListStore->set_sort_column(Gtk::TreeSortable::DEFAULT_UNSORTED_COLUMN_ID, Gtk::SortType::SORT_DESCENDING);
        ListStore->set_sort_column(Gtk::TreeSortable::DEFAULT_SORT_COLUMN_ID, Gtk::SortType::SORT_DESCENDING);
    }

    Gtk::TreeView& FriendList::Get()
    {
        return TreeView;
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
            AvatarRenderer.SetDefaultForeground(Modules::Friends::KairosMenuModule::GetRandomKairosAvatar());
            AvatarRenderer.SetDefaultBackground(Modules::Friends::KairosMenuModule::GetRandomKairosBackground());
            AvatarRenderer.SetDefaultIcon(Web::Xmpp::Status::Offline);

            AvatarRenderer.GetForegroundUrl.Set(Modules::Friends::KairosMenuModule::GetKairosAvatarUrl);
            AvatarRenderer.GetBackgroundUrl.Set(Modules::Friends::KairosMenuModule::GetKairosBackgroundUrl);
            AvatarRenderer.GetIconUrl.Set(Web::Xmpp::StatusToUrl);
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

            ProductRenderer.GetForegroundUrl.Set(Web::Epic::Friends::Presence::NamespacePresence::GetProductImageUrl);
            ProductRenderer.GetIconUrl.Set(Web::Epic::Friends::Presence::NamespacePresence::GetPlatformImageUrl);
        }
    }

    void FriendList::UpdateFriendRow(const Gtk::TreeRow& Row)
    {
        auto& FriendContainer = *(EGL3::Storage::Models::Friend*)Row[Columns.Data];
        auto& Friend = FriendContainer.Get();

        Row[Columns.DisplayNameMarkup] = std::format("<b>{}</b> <small><i>{}</i></small>", (std::string)Glib::Markup::escape_text(Friend.GetDisplayName()), (std::string)Glib::Markup::escape_text(Friend.GetSecondaryName()));

        Row[Columns.KairosAvatar] = Modules::Friends::KairosMenuModule::GetRandomKairosAvatar();
        Row[Columns.KairosBackground] = Modules::Friends::KairosMenuModule::GetRandomKairosBackground();

        if (FriendContainer.GetType() == Storage::Models::FriendType::NORMAL || FriendContainer.GetType() == Storage::Models::FriendType::CURRENT) {
            auto& FriendData = FriendContainer.Get<Storage::Models::FriendReal>();

            Row[Columns.Status] = FriendData.GetStatus();

            if (FriendData.GetStatus() != Web::Xmpp::Status::Offline) {
                Row[Columns.Product] = FriendData.GetProductId();
                Row[Columns.Platform] = FriendData.GetPlatformId();

                if (FriendData.GetStatusText().empty()) {
                    Row[Columns.Description] = Web::Xmpp::StatusToHumanString(FriendData.GetStatus());
                }
                else {
                    Row[Columns.Description] = FriendData.GetStatusText();
                    // TODO: add this as a tooltip too (somehow...)
                }
            }
            else {
                Row[Columns.Product] = "";
                Row[Columns.Platform] = "";

                Row[Columns.Description] = Web::Xmpp::StatusToHumanString(Web::Xmpp::Status::Offline);
            }
        }
        else {
            Row[Columns.Status] = Web::Xmpp::Status::Offline;

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

    FriendList::CellType FriendList::GetCellType(const Gtk::CellRenderer* Renderer) const noexcept
    {
        if (Renderer == &AvatarRenderer) {
            return CellType::Avatar;
        }
        else if (Renderer == &CenterTextRenderer) {
            return CellType::CenterText;
        }
        else if (Renderer == &ProductRenderer) {
            return CellType::Product;
        }
        else {
            return CellType::Invalid;
        }
    }
}
