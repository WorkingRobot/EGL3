#include "Chooser.h"

#include "../../web/xmpp/PresenceKairosProfile.h"

namespace EGL3::Modules::Login {
    using namespace Web::Xmpp;

    ChooserModule::ChooserModule(ModuleList& Ctx) :
        Icon(Ctx.GetWidget<Gtk::Image>("AccountChooserIcon")),
        TreeView(Ctx.GetWidget<Gtk::TreeView>("AccountChooserTree")),
        AvatarRenderer(TreeView, Ctx.GetModule<Modules::ImageCacheModule>(), 64, 0, 64),
        RecentlyCleared(true),
        RemoveState(false)
    {
        Icon.set(Icon.get_pixbuf()->scale_simple(96, 96, Gdk::INTERP_BILINEAR));

        ListStore = Gtk::ListStore::create(Columns);
        ListFilter = Gtk::TreeModelFilter::create(ListStore);
        ListFilter->set_visible_column(Columns.IsVisible);

        TreeView.set_model(ListFilter);

        {
            int ColCount = TreeView.append_column("Avatar", AvatarRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->add_attribute(AvatarRenderer.property_foreground(), Columns.KairosAvatar);
                Column->add_attribute(AvatarRenderer.property_background(), Columns.KairosBackground);
            }
            AvatarRenderer.SetDefaultForeground(Json::PresenceKairosProfile::GetDefaultKairosAvatar());
            AvatarRenderer.SetDefaultBackground(Json::PresenceKairosProfile::GetDefaultKairosBackground());

            AvatarRenderer.GetForegroundUrl.Set(Json::PresenceKairosProfile::GetKairosAvatarUrl);
            AvatarRenderer.GetBackgroundUrl.Set(Json::PresenceKairosProfile::GetKairosBackgroundUrl);
        }

        {
            int ColCount = TreeView.append_column("DisplayName", UsernameRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->add_attribute(UsernameRenderer.property_text(), Columns.DisplayName);
            }
            UsernameRenderer.property_scale() = 1.50;
            UsernameRenderer.set_padding(5, 0);
        }

        {
            int ColCount = TreeView.append_column("Login", ButtonRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->add_attribute(ButtonRenderer.property_icon_name(), Columns.ButtonIconName);
            }
            ButtonRenderer.property_scale() = 2;
            ButtonRenderer.property_xalign() = 1;
            ButtonRenderer.set_padding(5, 0);
        }

        TreeView.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_SINGLE);
        TreeView.get_selection()->set_select_function([this](const Glib::RefPtr<Gtk::TreeModel>& Model, const Gtk::TreeModel::Path& Path, bool CurrentlySelected) {
            // When the store is cleared, it automatically selects the first
            // item added, no matter what. This disables that behavior.
            if (RecentlyCleared) {
                RecentlyCleared = false;
                return false;
            }
            return true;
        });

        TreeView.signal_button_release_event().connect([this](GdkEventButton* Event) {
            auto Itr = TreeView.get_selection()->get_selected();
            if (!Itr) {
                return false;
            }

            auto& Row = *Itr;
            switch (Row[Columns.Type])
            {
            case RowType::Account:
            {
                auto& User = *Row[Columns.Data];
                if (RemoveState) {
                    AccountRemoved(&User);
                    ListStore->erase(ListFilter->convert_iter_to_child_iter(Itr));
                }
                else {
                    AccountClicked(&User);
                }
                break;
            }
            case RowType::Add:
                AccountAddRequest();
                break;
            case RowType::Remove:
                if (RemoveState) {
                    DisableRemoveState();
                }
                else {
                    EnableRemoveState();
                }
                break;
            }
            return false;
        });
    }

    void ChooserModule::AddAccount(Storage::Models::AuthUserData* User)
    {
        if (User == nullptr) {
            return;
        }

        auto Itr = ListStore->append();
        auto& Row = *Itr;
        Row[Columns.Data] = User;
        Row[Columns.KairosAvatar] = User->KairosAvatar;
        Row[Columns.KairosBackground] = User->KairosBackground;
        Row[Columns.DisplayName] = User->DisplayName;
        Row[Columns.ButtonIconName] = "go-next-symbolic";
        Row[Columns.IsVisible] = true;
        Row[Columns.Type] = RowType::Account;
    }

    void ChooserModule::FinalizeList(size_t AccountsSize)
    {
        {
            auto Itr = ListStore->append();
            auto& Row = *Itr;
            Row[Columns.Data] = nullptr;
            Row[Columns.KairosAvatar] = "cid_069_168fcd0487d81a9e4342f63bb86a7f96f394245e737593468d644f03ab0d4dee";
            Row[Columns.KairosBackground] = "[\"#FFB4D6\",\"#FF619C\",\"#7D3449\"]";
            Row[Columns.DisplayName] = "Add New Account";
            Row[Columns.ButtonIconName] = "list-add-symbolic";
            Row[Columns.IsVisible] = true;
            Row[Columns.Type] = RowType::Add;
        }
        {
            auto Itr = ListStore->append();
            auto& Row = *Itr;
            Row[Columns.Data] = nullptr;
            Row[Columns.KairosAvatar] = "cid_082_0d16594844aaf684ad3d1b7ace0b599f4c020b66c12147d01cdf5e2ddc84b4f2";
            Row[Columns.KairosBackground] = "[\"#F16712\",\"#D8033C\",\"#6E0404\"]";
            Row[Columns.DisplayName] = "Remove Account";
            Row[Columns.ButtonIconName] = "list-remove-symbolic";
            Row[Columns.IsVisible] = AccountsSize != 0;
            Row[Columns.Type] = RowType::Remove;
        }
    }

    void ChooserModule::ClearList()
    {
        ListStore->clear();
        RecentlyCleared = true;
    }

    void ChooserModule::EnableRemoveState()
    {
        if (RemoveState) {
            return;
        }
        RemoveState = true;

        ListStore->foreach_iter([this](const Gtk::TreeModel::iterator& Itr) {
            auto& Row = *Itr;
            switch (Row[Columns.Type]) {
            case RowType::Account:
                Row[Columns.ButtonIconName] = "edit-delete-symbolic";
                break;
            case RowType::Add:
                Row[Columns.IsVisible] = false;
                break;
            case RowType::Remove:
                Row[Columns.DisplayName] = "Cancel";
                Row[Columns.ButtonIconName] = "edit-undo-symbolic";
                break;
            }
            return false;
        });
    }

    void ChooserModule::DisableRemoveState()
    {
        if (!RemoveState) {
            return;
        }
        RemoveState = false;

        ListStore->foreach_iter([this](const Gtk::TreeModel::iterator& Itr) {
            auto& Row = *Itr;
            switch (Row[Columns.Type]) {
            case RowType::Account:
                Row[Columns.ButtonIconName] = "go-next-symbolic";
                break;
            case RowType::Add:
                Row[Columns.IsVisible] = true;
                break;
            case RowType::Remove:
            {
                Row[Columns.DisplayName] = "Remove Account";
                Row[Columns.ButtonIconName] = "list-remove-symbolic";
                size_t Count = 0;
                ListStore->foreach_iter([this, &Count](const Gtk::TreeModel::iterator& Itr) {
                    auto& Row = *Itr;
                    if (Row[Columns.Data] != nullptr) {
                        ++Count;
                    }
                    return false;
                });
                if (Count != 0) {
                    Row[Columns.IsVisible] = true;
                    TreeView.scroll_to_row(ListFilter->get_path(ListFilter->convert_child_iter_to_iter(Itr)));
                }
                else {
                    Row[Columns.IsVisible] = false;
                }
                break;
            }
            }
            return false;
        });

    }
}