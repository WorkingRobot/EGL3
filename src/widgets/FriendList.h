#pragma once

#include "../storage/models/Friend.h"
#include "../utils/SlotHolder.h"
#include "CellRendererAvatarStatus.h"
#include "CellRendererCenterText.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class FriendList {
    public:
        enum class CellType : uint8_t {
            Avatar,
            CenterText,
            Product,

            Count,
            Invalid
        };

        FriendList(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache);

        ~FriendList();

        template<class Sort, class Filt>
        FriendList(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache, Sort Sorter, Filt Filter) :
            FriendList(TreeView, ImageCache)
        {
            ListStore->set_default_sort_func([this, Sorter](const Gtk::TreeModel::iterator& AItr, const Gtk::TreeModel::iterator& BItr) -> int {
                auto& ARow = *AItr;
                auto& BRow = *BItr;
                auto APtr = (Storage::Models::Friend*)ARow[Columns.Data];
                auto BPtr = (Storage::Models::Friend*)BRow[Columns.Data];

                if (!APtr && !BPtr) {
                    return 0;
                }
                else if (!BPtr) {
                    return 1;
                }
                else if (!APtr) {
                    return -1;
                }

                std::weak_ordering Comp = Sorter(*APtr, *BPtr);

                return Comp._Value;
            });
            ListStore->set_sort_column(Gtk::TreeSortable::DEFAULT_SORT_COLUMN_ID, Gtk::SortType::SORT_DESCENDING);

            ListFilter->set_visible_func([this, Filter](const Gtk::TreeModel::const_iterator& Itr) {
                auto& Row = *Itr;
                auto Ptr = (Storage::Models::Friend*)Row[Columns.Data];

                if (!Ptr) {
                    return true;
                }

                return Filter(*Ptr);
            });
        }

        void Clear();

        void Add(Storage::Models::Friend& Friend);

        void Refilter();

        void Resort();

        Gtk::TreeView& Get();

        Utils::Callback<void(Storage::Models::Friend&, CellType, int, int, Gdk::Rectangle&)> FriendClicked;
        Utils::Callback<bool(Storage::Models::Friend&, CellType, int, int, const Glib::RefPtr<Gtk::Tooltip>&)> FriendTooltipped;

    private:
        void SetupColumns();

        void UpdateFriendRow(const Gtk::TreeRow& Row);

        CellType GetCellType(const Gtk::CellRenderer* Renderer) const noexcept;

        Modules::ImageCacheModule& ImageCache;

        Gtk::TreeView& TreeView;

        Glib::RefPtr<Gtk::ListStore> ListStore;
        Glib::RefPtr<Gtk::TreeModelFilter> ListFilter;

        CellRendererAvatarStatus<std::string, std::string, EGL3::Web::Xmpp::Status> AvatarRenderer;
        CellRendererCenterText CenterTextRenderer;
        CellRendererAvatarStatus<std::string, void, std::string> ProductRenderer;

        Utils::SlotHolder SlotTooltip;
        Utils::SlotHolder SlotClick;

        struct ModelColumns : public Gtk::TreeModel::ColumnRecord
        {
            ModelColumns()
            {
                add(Data);
                add(UpdateSlot);
                add(KairosAvatar);
                add(KairosBackground);
                add(Status);
                add(DisplayNameMarkup);
                add(Description);
                add(Product);
                add(Platform);
            }

            Gtk::TreeModelColumn<Storage::Models::Friend*> Data;
            Gtk::TreeModelColumn<Utils::SharedSlotHolder> UpdateSlot;
            Gtk::TreeModelColumn<Glib::ustring> KairosAvatar;
            Gtk::TreeModelColumn<Glib::ustring> KairosBackground;
            Gtk::TreeModelColumn<EGL3::Web::Xmpp::Status> Status;
            Gtk::TreeModelColumn<Glib::ustring> DisplayNameMarkup;
            Gtk::TreeModelColumn<Glib::ustring> Description;
            Gtk::TreeModelColumn<Glib::ustring> Product;
            Gtk::TreeModelColumn<Glib::ustring> Platform;
        };
        ModelColumns Columns;
    };
}
