#pragma once

#include "../modules/Login/Auth.h"
#include "../modules/ImageCache.h"
#include "../utils/egl/RememberMe.h"
#include "../utils/SlotHolder.h"
#include "CellRendererAvatarStatus.h"
#include "CellRendererPixbufScalable.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class AccountList {
    public:
        AccountList(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache, Modules::Login::AuthModule& Auth, int AvatarSize, double TextScale);

        ~AccountList();

        void Refresh();

    private:
        void AddAccount(Storage::Models::AuthUserData* User);

        void FinalizeList(bool IsEmpty);

        void ClearList();

        void EnableRemoveState();

        void DisableRemoveState();

        Modules::ImageCacheModule& ImageCache;
        Modules::Login::AuthModule& Auth;

        Gtk::TreeView& TreeView;

        Widgets::CellRendererAvatarStatus<std::string, std::string, void> AvatarRenderer;
        Gtk::CellRendererText UsernameRenderer;
        Widgets::CellRendererPixbufScalable ButtonRenderer;
        Glib::RefPtr<Gtk::ListStore> ListStore;
        Glib::RefPtr<Gtk::TreeModelFilter> ListFilter;
        bool RecentlyCleared;

        Utils::SlotHolder SlotClicked;

        bool RemoveState;

        enum class RowType : uint8_t {
            Account,
            EGL,
            Add,
            Remove
        };

        struct ModelColumns : public Gtk::TreeModel::ColumnRecord
        {
            ModelColumns()
            {
                add(Data);
                add(KairosAvatar);
                add(KairosBackground);
                add(DisplayName);
                add(ButtonIconName);
                add(IsVisible);
                add(Type);
            }

            Gtk::TreeModelColumn<Storage::Models::AuthUserData*> Data;
            Gtk::TreeModelColumn<Glib::ustring> KairosAvatar;
            Gtk::TreeModelColumn<Glib::ustring> KairosBackground;
            Gtk::TreeModelColumn<Glib::ustring> DisplayName;
            Gtk::TreeModelColumn<Glib::ustring> ButtonIconName;
            Gtk::TreeModelColumn<bool> IsVisible;
            Gtk::TreeModelColumn<RowType> Type;
        };
        ModelColumns Columns;
    };
}
