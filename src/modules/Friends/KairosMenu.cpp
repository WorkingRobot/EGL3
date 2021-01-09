#include "KairosMenu.h"

#include "../../web/xmpp/PresenceKairosProfile.h"

namespace EGL3::Modules::Friends {
    using namespace Web::Xmpp;

    KairosMenuModule::KairosMenuModule(ModuleList& Modules, const Utils::GladeBuilder& Builder) :
        Auth(Modules.GetModule<AuthorizationModule>()),
        ImageCache(Modules.GetModule<ImageCacheModule>()),
        Options(Modules.GetModule<OptionsModule>()),
        Focused(false),
        Window(Builder.GetWidget<Gtk::Window>("FriendsKairosMenu")),
        AvatarBox(Builder.GetWidget<Gtk::FlowBox>("FriendsAvatarFlow")),
        BackgroundBox(Builder.GetWidget<Gtk::FlowBox>("FriendsBackgroundFlow")),
        StatusBox(Builder.GetWidget<Gtk::FlowBox>("FriendsStatusFlow")),
        StatusEntry(Builder.GetWidget<Gtk::Entry>("FriendsKairosStatusEntry")),
        StatusEditBtn(Builder.GetWidget<Gtk::Button>("FriendsKairosEditStatusBtn"))
    {
        {
            StatusBox.foreach([this](Gtk::Widget& Widget) { StatusBox.remove(Widget); });

            static const std::initializer_list<Json::ShowStatus> Statuses{ Json::ShowStatus::Online, Json::ShowStatus::Away, Json::ShowStatus::ExtendedAway, Json::ShowStatus::DoNotDisturb, Json::ShowStatus::Chat };

            StatusWidgets.clear();
            StatusWidgets.reserve(Statuses.size());
            for (auto Status : Statuses) {
                auto& Widget = StatusWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<Json::ShowStatus>>(Status, "", 48, 48, &Json::ShowStatusToUrl, ImageCache));
                StatusBox.add(*Widget);
            }

            StatusBox.show_all_children();
        }

        {
            Window.signal_show().connect([this]() {
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
                    auto Data = (Widgets::AsyncImageKeyed<Json::ShowStatus>*)Widget.get_child()->get_data("EGL3_ImageBase");
                    if (Data->GetKey() == GetCurrentUser().GetShowStatus()) {
                        StatusBox.select_child(Widget);
                    }
                });

                StatusEntry.set_placeholder_text(ShowStatusToString(Options.GetStorageData().GetShowStatus()));
                StatusEntry.set_text(Options.GetStorageData().GetStatus());
                StatusEditBtn.set_sensitive(false);
            });
            Window.signal_focus_out_event().connect([this](GdkEventFocus* evt) {
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
            AvatarBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
                auto Data = (Widgets::AsyncImageKeyed<std::string>*)child->get_child()->get_data("EGL3_ImageBase");
                if (Data->GetKey() == GetCurrentUser().GetKairosAvatar()) {
                    return;
                }

                GetCurrentUser().SetKairosAvatar(Data->GetKey());
                UpdateXmppPresence();

                UpdateAvatarTask = std::async(std::launch::async, [this]() {
                    auto UpdateResp = Auth.GetClientLauncher().UpdateAccountSetting("avatar", GetCurrentUser().GetKairosAvatar());
                    if (UpdateResp.HasError()) {
                        EGL3_LOG(LogLevel::Error, "Updating kairos avatar encountered an error");
                    }
                });
            });
            BackgroundBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
                auto Data = (Widgets::AsyncImageKeyed<std::string>*)child->get_child()->get_data("EGL3_ImageBase");
                if (Data->GetKey() == GetCurrentUser().GetKairosBackground()) {
                    return;
                }

                GetCurrentUser().SetKairosBackground(Data->GetKey());
                UpdateXmppPresence();

                UpdateBackgroundTask = std::async(std::launch::async, [this]() {
                    auto UpdateResp = Auth.GetClientLauncher().UpdateAccountSetting("avatarBackground", GetCurrentUser().GetKairosBackground());
                    if (UpdateResp.HasError()) {
                        EGL3_LOG(LogLevel::Error, "Updating kairos background encountered an error");
                    }
                });
            });
            StatusBox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
                auto Data = (Widgets::AsyncImageKeyed<Json::ShowStatus>*)child->get_child()->get_data("EGL3_ImageBase");
                if (Data->GetKey() == GetCurrentUser().GetShowStatus()) {
                    return;
                }

                Options.GetStorageData().SetShowStatus(Data->GetKey());
                StatusEntry.set_placeholder_text(ShowStatusToString(Data->GetKey()));
                GetCurrentUser().SetShowStatus(Data->GetKey());
                UpdateXmppPresence();
            });

            StatusEntry.signal_changed().connect([this]() {
                StatusEditBtn.set_sensitive(true);
            });

            StatusEditBtn.signal_clicked().connect([this]() {
                StatusEditBtn.set_sensitive(false);

                Options.GetStorageData().SetStatus(StatusEntry.get_text());
                GetCurrentUser().SetDisplayStatus(StatusEntry.get_text());
                UpdateXmppPresence();
            });
        }
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
                auto& Widget = AvatarsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Avatar, Json::PresenceKairosProfile::GetDefaultKairosAvatarUrl(), 64, 64, &Json::PresenceKairosProfile::GetKairosAvatarUrl, ImageCache));
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
                auto& Widget = BackgroundsWidgets.emplace_back(std::make_unique<Widgets::AsyncImageKeyed<std::string>>(Background, Json::PresenceKairosProfile::GetDefaultKairosBackgroundUrl(), 64, 64, &Json::PresenceKairosProfile::GetKairosBackgroundUrl, ImageCache));
                BackgroundBox.add(*Widget);
            }

            BackgroundBox.show_all_children();
        }
    }
}