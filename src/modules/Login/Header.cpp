#include "Header.h"

namespace EGL3::Modules::Login {
    HeaderModule::HeaderModule(ModuleList& Ctx) :
        Auth(Ctx.GetModule<AuthModule>()),
        Switcher(Ctx.GetWidget<Gtk::StackSwitcher>("MainStackSwitcher")),
        LoginInfo(Ctx.GetWidget<Gtk::Box>("LoginInfoContainer")),
        Username(Ctx.GetWidget<Gtk::Label>("LoginInfoUsername")),
        AvatarContainer(Ctx.GetWidget<Gtk::AspectFrame>("LoginInfoAvatarContainer")),
        SelectIconRevealer(Ctx.GetWidget<Gtk::Revealer>("LoginInfoSelectIconRevealer")),
        EventBox(Ctx.GetWidget<Gtk::EventBox>("LoginInfoEventBox")),
        Avatar(Ctx.GetModule<Modules::ImageCacheModule>(), 32),
        Focused(false),
        AccountListWindow(Ctx.GetWidget<Gtk::Window>("LoginInfoMenu")),
        AccountList(Ctx.GetWidget<Gtk::TreeView>("LoginInfoTree"), Ctx.GetModule<ImageCacheModule>(), Auth, 48, 1.00)
    {
        EventBox.signal_enter_notify_event().connect([this](GdkEventCrossing* Event) {
            SelectIconRevealer.set_reveal_child(true);
            LoginInfo.set_state_flags(Gtk::STATE_FLAG_PRELIGHT, false);
            return false;
        });
        EventBox.signal_leave_notify_event().connect([this](GdkEventCrossing* Event) {
            SelectIconRevealer.set_reveal_child(false);
            LoginInfo.unset_state_flags(Gtk::STATE_FLAG_PRELIGHT);
            return false;
        });
        EventBox.signal_button_release_event().connect([this](GdkEventButton* Event) {
            AccountList.Refresh();

            AccountListWindow.set_attached_to(LoginInfo);
            AccountListWindow.show();
            AccountListWindow.present();

            auto Alloc = LoginInfo.get_allocation();
            int x, y;
            LoginInfo.get_window()->get_origin(x, y);
            // Moved after show/present because get_height returns 1 when hidden (couldn't find other workarounds for that, and I'm not bothered since there's no flickering)
            AccountListWindow.move(x + Alloc.get_x() - 4, y + Alloc.get_y() + Alloc.get_height());
            return false;
        });

        LoggedInDispatcher.connect([this]() {
            Show(*Auth.GetSelectedUserData());
        });

        Auth.LoggedIn.connect([this]() {
            LoggedInDispatcher.emit();
        });

        Auth.LoggedOut.connect([this]() {
            AccountListWindow.hide();
            Hide();
        });

        {
            AccountListWindow.signal_show().connect([this]() {
                Focused = false;
            });
            AccountListWindow.signal_focus_out_event().connect([this](GdkEventFocus* evt) {
                if (!Focused) {
                    // This is always called once for some reason
                    // The first event is a fake
                    Focused = true;
                }
                else {
                    AccountListWindow.hide();
                }
                return false;
            });
        }

        AvatarContainer.add(Avatar);
        Avatar.show();

        Hide();
    }

    void HeaderModule::Hide()
    {
        Switcher.hide();
        LoginInfo.hide();
    }

    void HeaderModule::Show(const Storage::Models::AuthUserData& Data)
    {
        Switcher.show();
        Username.set_text(Data.DisplayName);
        Avatar.set_avatar(Data.KairosAvatar, Data.KairosBackground);
        LoginInfo.show();
    }
}