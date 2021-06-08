#pragma once

#include "../ModuleList.h"
#include "../../storage/models/Authorization.h"
#include "../../widgets/CellRendererAvatarStatus.h"
#include "../../widgets/CellRendererPixbufScalable.h"
#include "../../utils/Callback.h"

#include <gtkmm.h>

namespace EGL3::Modules::Login {
    class ChooserModule : public BaseModule {
    public:
        ChooserModule(ModuleList& Ctx);

        template<class It>
        void SetAccounts(It Begin, It End)
        {
            ClearList();
            std::for_each(Begin, End, [this](Storage::Models::AuthUserData& Data) { AddAccount(&Data); });
            FinalizeList(std::distance(Begin, End));
        }

        Utils::Callback<void(Storage::Models::AuthUserData*)> AccountClicked;
        Utils::Callback<void(Storage::Models::AuthUserData*)> AccountRemoved;
        Utils::Callback<void()> AccountAddRequest;

    private:
        void AddAccount(Storage::Models::AuthUserData* User);

        void FinalizeList(size_t AccountsSize);

        void ClearList();

        void EnableRemoveState();

        void DisableRemoveState();

        Gtk::Image& Icon;
        Gtk::TreeView& TreeView;

        Widgets::CellRendererAvatarStatus<std::string, std::string, void> AvatarRenderer;
        Gtk::CellRendererText UsernameRenderer;
        Widgets::CellRendererPixbufScalable ButtonRenderer;
        Glib::RefPtr<Gtk::ListStore> ListStore;
        Glib::RefPtr<Gtk::TreeModelFilter> ListFilter;
        bool RecentlyCleared;

        bool RemoveState;

        enum class RowType : uint8_t {
            Account,
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