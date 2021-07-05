#include "KairosMenu.h"

#include "List.h"

namespace EGL3::Modules::Friends {
    KairosMenuModule::KairosMenuModule(ModuleList& Ctx) :
        Auth(Ctx.GetModule<Login::AuthModule>()),
        Header(Ctx.GetModule<Login::HeaderModule>()),
        ImageCache(Ctx.GetModule<ImageCacheModule>()),
        List(Ctx.GetModule<ListModule>()),
        Options(Ctx.GetModule<OptionsModule>()),
        Focused(false),
        Window(Ctx.GetWidget<Gtk::Window>("FriendsKairosMenu")),
        AvatarBox(Ctx.GetWidget<Gtk::FlowBox>("FriendsAvatarFlow")),
        BackgroundBox(Ctx.GetWidget<Gtk::FlowBox>("FriendsBackgroundFlow")),
        StatusBox(Ctx.GetWidget<Gtk::FlowBox>("FriendsStatusFlow")),
        StatusEntry(Ctx.GetWidget<Gtk::Entry>("FriendsKairosStatusEntry")),
        StatusEditBtn(Ctx.GetWidget<Gtk::Button>("FriendsKairosEditStatusBtn"))
    {
        {
            StatusBox.foreach([this](Gtk::Widget& Widget) { StatusBox.remove(Widget); });

            static const std::initializer_list<Web::Xmpp::Status> Statuses{ Web::Xmpp::Status::Online, Web::Xmpp::Status::Away, Web::Xmpp::Status::ExtendedAway, Web::Xmpp::Status::DoNotDisturb };

            StatusWidgets.clear();
            StatusWidgets.reserve(Statuses.size());
            for (auto Status : Statuses) {
                auto& Widget = StatusWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<Web::Xmpp::Status>>(Status, Web::Xmpp::Status::Offline, 48, 48, &Web::Xmpp::StatusToUrl, ImageCache));
                StatusBox.add(*Widget);
            }

            StatusBox.show_all_children();
        }

        {
            SlotWindowShown = Window.signal_show().connect([this]() {
                Focused = false;

                AvatarBox.foreach([this](Gtk::Widget& WidgetBase) {
                    auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
                    auto Data = (Widgets::AsyncImageKeyed<std::string>*)Widget.get_child()->get_data("EGL3_ImageBase");
                    if (Data->GetKey() == GetCurrentUser().GetKairosAvatar()) {
                        AvatarBox.select_child(Widget);
                    }
                });

                BackgroundBox.foreach([this](Gtk::Widget& WidgetBase) {
                    auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
                    auto Data = (Widgets::AsyncImageKeyed<std::string>*)Widget.get_child()->get_data("EGL3_ImageBase");
                    if (Data->GetKey() == GetCurrentUser().GetKairosBackground()) {
                        BackgroundBox.select_child(Widget);
                    }
                });

                StatusBox.foreach([this](Gtk::Widget& WidgetBase) {
                    auto& Widget = (Gtk::FlowBoxChild&)WidgetBase;
                    auto Data = (Widgets::AsyncImageKeyed<Web::Xmpp::Status>*)Widget.get_child()->get_data("EGL3_ImageBase");
                    if (Data->GetKey() == GetCurrentUser().GetStatus()) {
                        StatusBox.select_child(Widget);
                    }
                });

                StatusEntry.set_placeholder_text(Web::Xmpp::StatusToHumanString(Options.GetStatus()));
                StatusEntry.set_text(Options.GetStatusText());
                StatusEditBtn.set_sensitive(false);
            });
            SlotWindowUnfocused = Window.signal_focus_out_event().connect([this](GdkEventFocus* evt) {
                if (!Focused) {
                    // This is always called once for some reason
                    // The first event is a fake
                    Focused = true;
                }
                else {
                    Window.hide();
                }
                return false;
            });
        }

        {
            SlotAvatarClicked = AvatarBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
                auto Data = (Widgets::AsyncImageKeyed<std::string>*)child->get_child()->get_data("EGL3_ImageBase");
                if (Data->GetKey() == GetCurrentUser().GetKairosAvatar()) {
                    return;
                }

                GetCurrentUser().SetKairosAvatar(Data->GetKey());

                Auth.GetSelectedUserData()->KairosAvatar = Data->GetKey();
                Header.Show(*Auth.GetSelectedUserData());

                UpdateAvatarTask = std::async(std::launch::async, [this]() {
                    auto UpdateResp = Auth.GetClientLauncher().UpdateAccountSetting("avatar", GetCurrentUser().GetKairosAvatar());
                    if (UpdateResp.HasError()) {
                        EGL3_LOG(LogLevel::Error, "Updating kairos avatar encountered an error");
                    }
                });
            });
            SlotBackgroundClicked = BackgroundBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
                auto Data = (Widgets::AsyncImageKeyed<std::string>*)child->get_child()->get_data("EGL3_ImageBase");
                if (Data->GetKey() == GetCurrentUser().GetKairosBackground()) {
                    return;
                }

                GetCurrentUser().SetKairosBackground(Data->GetKey());

                Auth.GetSelectedUserData()->KairosBackground = Data->GetKey();
                Header.Show(*Auth.GetSelectedUserData());

                UpdateBackgroundTask = std::async(std::launch::async, [this]() {
                    auto UpdateResp = Auth.GetClientLauncher().UpdateAccountSetting("avatarBackground", GetCurrentUser().GetKairosBackground());
                    if (UpdateResp.HasError()) {
                        EGL3_LOG(LogLevel::Error, "Updating kairos background encountered an error");
                    }
                });
            });
            SlotStatusClicked = StatusBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
                auto Data = (Widgets::AsyncImageKeyed<Web::Xmpp::Status>*)child->get_child()->get_data("EGL3_ImageBase");
                if (Data->GetKey() == GetCurrentUser().GetStatus()) {
                    return;
                }

                Options.SetStatus(Data->GetKey());
                StatusEntry.set_placeholder_text(Web::Xmpp::StatusToHumanString(Data->GetKey()));
                GetCurrentUser().SetStatus(Data->GetKey());
                OnUpdatePresence();
            });

            SlotStatusTextChanged = StatusEntry.signal_changed().connect([this]() {
                StatusEditBtn.set_sensitive(true);
            });

            SlotStatusTextClicked = StatusEditBtn.signal_clicked().connect([this]() {
                StatusEditBtn.set_sensitive(false);

                Options.SetStatusText(StatusEntry.get_text());
                GetCurrentUser().SetStatusText(StatusEntry.get_text());
                OnUpdatePresence();
            });
        }

        List.SetKairosMenuWindow(GetWindow());
    }

    Gtk::Window& KairosMenuModule::GetWindow() const
    {
        return Window;
    }

    void KairosMenuModule::SetAvailableSettings(std::vector<std::string>&& Avatars, std::vector<std::string>&& Backgrounds)
    {
        AvatarsData = std::move(Avatars);
        BackgroundsData = std::move(Backgrounds);
    }

    void KairosMenuModule::UpdateAvailableSettings()
    {
        // KairosAvatarsResp handling
        {
            AvatarBox.foreach([this](Gtk::Widget& Widget) { AvatarBox.remove(Widget); });

            AvatarsWidgets.clear();
            AvatarsWidgets.reserve(AvatarsData.size());

            for (auto& Avatar : AvatarsData) {
                auto& Widget = AvatarsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Avatar, GetDefaultKairosAvatar(), 64, 64, &GetKairosAvatarUrl, ImageCache));
                AvatarBox.add(*Widget);
            }

            AvatarBox.show_all_children();
        }

        // KairosBackgroundsResp handling
        {
            BackgroundBox.foreach([this](Gtk::Widget& Widget) { BackgroundBox.remove(Widget); });

            BackgroundsWidgets.clear();
            BackgroundsWidgets.reserve(BackgroundsData.size());

            for (auto& Background : BackgroundsData) {
                auto& Widget = BackgroundsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Background, GetDefaultKairosBackground(), 64, 64, &GetKairosBackgroundUrl, ImageCache));
                BackgroundBox.add(*Widget);
            }

            BackgroundBox.show_all_children();
        }
    }

    std::string KairosMenuModule::GetDefaultKairosAvatar() {
        uint32_t Id = Utils::Random(1, 8);
        return std::format("cid_{:03}_athena_commando_{}_default", Id, Id > 4 ? 'm' : 'f');
    }

    std::string KairosMenuModule::GetKairosAvatarUrl(const std::string& Avatar) {
        if (Avatar.empty()) {
            return GetKairosAvatarUrl(GetDefaultKairosAvatar());
        }
        return std::format("{}Kairos/portraits/{}.png?preview=1", Web::GetHostUrl<Web::Host::UnrealEngineCdn2>(), Avatar);
    }

    std::string KairosMenuModule::GetDefaultKairosBackground() {
        return "[\"#AEC1D3\",\"#687B8E\",\"#36404A\"]";
    }

    std::string KairosMenuModule::GetKairosBackgroundUrl(const std::string& Background) {
        if (Background.empty()) {
            return GetKairosBackgroundUrl(GetDefaultKairosBackground());
        }
        auto Hash = Utils::Crc32(Background.c_str(), Background.size());
        return std::format("{}backgrounds/{:04X}.png", Web::GetHostUrl<Web::Host::EGL3>(), Hash);
    }

    Storage::Models::FriendCurrent& KairosMenuModule::GetCurrentUser() const
    {
        return List.GetCurrentUser();
    }
}