#include "AccountList.h"

#include "../modules/Friends/KairosMenu.h"

namespace EGL3::Widgets {
    AccountList::AccountList(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache, Modules::Login::AuthModule& Auth, int AvatarSize, double TextScale) :
        ImageCache(ImageCache),
        Auth(Auth),
        TreeView(TreeView),
        AvatarRenderer(TreeView, ImageCache, AvatarSize, 0, AvatarSize),
        RecentlyCleared(true),
        RemoveState(false)
    {
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
            AvatarRenderer.SetDefaultForeground(Modules::Friends::KairosMenuModule::GetRandomKairosAvatar());
            AvatarRenderer.SetDefaultBackground(Modules::Friends::KairosMenuModule::GetRandomKairosBackground());

            AvatarRenderer.GetForegroundUrl.Set(Modules::Friends::KairosMenuModule::GetKairosAvatarUrl);
            AvatarRenderer.GetBackgroundUrl.Set(Modules::Friends::KairosMenuModule::GetKairosBackgroundUrl);
        }

        {
            int ColCount = TreeView.append_column("DisplayName", UsernameRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->set_expand(true);

                Column->add_attribute(UsernameRenderer.property_text(), Columns.DisplayName);
            }
            UsernameRenderer.property_scale() = TextScale;
            UsernameRenderer.set_padding(5, 0);
        }

        {
            int ColCount = TreeView.append_column("Login", ButtonRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->set_fixed_width(AvatarSize);

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

        SlotClicked = TreeView.signal_button_release_event().connect([this](GdkEventButton* Event) {
            auto Itr = this->TreeView.get_selection()->get_selected();
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
                    if (this->Auth.AccountRemoved(User.AccountId)) {
                        ListStore->erase(ListFilter->convert_iter_to_child_iter(Itr));
                    }
                }
                else {
                    this->Auth.AccountSelected(User);
                }
                break;
            }
            case RowType::EGL:
                this->Auth.AccountSelectedEGL();
                break;
            case RowType::Add:
                this->Auth.OpenSignInPage();
                break;
            case RowType::Remove:
                if (RemoveState) {
                    DisableRemoveState();
                }
                else {
                    EnableRemoveState();
                }
                break;
            case RowType::LogOut:
                this->Auth.LogOut();
                break;
            }
            return false;
        });

        Refresh();
    }

    AccountList::~AccountList()
    {
        TreeView.remove_all_columns();
        TreeView.unset_model();
    }

    void AccountList::Refresh()
    {
        ClearList();
        auto& UserData = Auth.GetUserData();
        std::for_each(UserData.begin(), UserData.end(), [this](Storage::Models::AuthUserData& Data) { AddAccount(&Data); });
        FinalizeList(UserData.empty());
    }

    void AccountList::AddAccount(Storage::Models::AuthUserData* User)
    {
        if (User == nullptr || Auth.GetSelectedUserData() == User) {
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

    void AccountList::FinalizeList(bool IsEmpty)
    {
        {
            auto ProfilePtr = Auth.GetRememberMe().GetProfile();
            if (ProfilePtr != nullptr) {
                auto& Profile = *ProfilePtr;
                bool ProfileFound = false;

                if (!ProfileFound) {
                    auto SelectedUser = Auth.GetSelectedUserData();
                    if (SelectedUser != nullptr) {
                        ProfileFound = SelectedUser->DisplayName == Profile.DisplayName;
                    }
                }

                if (!ProfileFound) {
                    ListStore->foreach_iter([this, &ProfileFound, &Profile](const Gtk::TreeModel::iterator& Itr) {
                        auto& Row = *Itr;
                        auto Data = (Storage::Models::AuthUserData*)Row[Columns.Data];
                        return ProfileFound = Data->DisplayName == Profile.DisplayName;
                    });
                }

                if (!ProfileFound) {
                    auto Itr = ListStore->append();
                    auto& Row = *Itr;
                    Row[Columns.Data] = nullptr;
                    Row[Columns.KairosAvatar] = "cid_486_741d7965509eb9b4f859e758d12a23ea8b30221d5a3a6b731dbbe471bb7b4138";
                    Row[Columns.KairosBackground] = "[\"#E93FEB\",\"#7B009C\",\"#500066\"]";
                    Row[Columns.DisplayName] = Profile.DisplayName + " (EGL)";
                    Row[Columns.ButtonIconName] = "go-next-symbolic";
                    Row[Columns.IsVisible] = true;
                    Row[Columns.Type] = RowType::EGL;
                }
            }
        }

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
            Row[Columns.IsVisible] = !IsEmpty;
            Row[Columns.Type] = RowType::Remove;
        }

        {
            auto User = Auth.GetSelectedUserData();
            if (User != nullptr) {
                auto Itr = ListStore->append();
                auto& Row = *Itr;
                Row[Columns.Data] = User;
                Row[Columns.KairosAvatar] = User->KairosAvatar;
                Row[Columns.KairosBackground] = User->KairosBackground;
                Row[Columns.DisplayName] = "Log Out";
                Row[Columns.ButtonIconName] = "system-log-out-symbolic";
                Row[Columns.IsVisible] = true;
                Row[Columns.Type] = RowType::LogOut;
            }
        }
    }

    void AccountList::ClearList()
    {
        ListStore->clear();
        RecentlyCleared = true;
    }

    void AccountList::EnableRemoveState()
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
            case RowType::EGL:
            case RowType::Add:
            case RowType::LogOut:
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

    void AccountList::DisableRemoveState()
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
            case RowType::EGL:
            case RowType::Add:
            case RowType::LogOut:
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
