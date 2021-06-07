#pragma once

#include "../storage/models/Friend.h"
#include "CellRendererAvatarStatus.h"
#include "CellRendererCenterText.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class FriendList {
    public:
        FriendList(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache);

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
            ListStore->set_sort_column(GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, Gtk::SortType::SORT_DESCENDING);

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

        Utils::Callback<void(Storage::Models::Friend&)> FriendClicked;

    private:
        void SetupColumns();

        void UpdateFriendRow(const Gtk::TreeRow& Row);

        Modules::ImageCacheModule& ImageCache;

        Gtk::TreeView& TreeView;

        Glib::RefPtr<Gtk::ListStore> ListStore;
        Glib::RefPtr<Gtk::TreeModelFilter> ListFilter;

        CellRendererAvatarStatus<std::string, std::string, EGL3::Web::Xmpp::Json::ShowStatus> AvatarRenderer;
        CellRendererCenterText CenterTextRenderer;
        CellRendererAvatarStatus<std::string, void, std::string> ProductRenderer;

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
    };
}
