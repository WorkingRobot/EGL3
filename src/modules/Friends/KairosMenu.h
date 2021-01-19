#pragma once

#include "../../storage/models/FriendCurrent.h"
#include "../../utils/Callback.h"
#include "../../utils/GladeBuilder.h"
#include "../../web/xmpp/ShowStatus.h"
#include "../../widgets/AsyncImageKeyed.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../Authorization.h"
#include "../ImageCache.h"
#include "Options.h"
#include "List.h"

#include <functional>
#include <future>
#include <gtkmm.h>
#include <optional>

namespace EGL3::Modules::Friends {
    class KairosMenuModule : public BaseModule {
    public:
        KairosMenuModule(ModuleList& Modules, const Utils::GladeBuilder& Builder);

        Gtk::Window& GetWindow() const;

        void SetAvailableSettings(std::vector<std::string>&& Avatars, std::vector<std::string>&& Backgrounds);

        void UpdateAvailableSettings();

        Utils::Callback<void()> UpdateXmppPresence;

    private:
        Storage::Models::FriendCurrent& GetCurrentUser() const;

        AuthorizationModule& Auth;
        ImageCacheModule& ImageCache;

        ListModule& List;
        OptionsModule& Options;

        bool Focused;
        Gtk::Window& Window;
        Gtk::FlowBox& AvatarBox;
        Gtk::FlowBox& BackgroundBox;
        Gtk::FlowBox& StatusBox;
        Gtk::Entry& StatusEntry;
        Gtk::Button& StatusEditBtn;

        std::future<void> UpdateAvatarTask;
        std::future<void> UpdateBackgroundTask;

        std::vector<std::string> AvatarsData;
        std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<std::string>>> AvatarsWidgets;

        std::vector<std::string> BackgroundsData;
        std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<std::string>>> BackgroundsWidgets;

        std::vector<std::unique_ptr<Widgets::AsyncImageKeyed<Web::Xmpp::Json::ShowStatus>>> StatusWidgets;
    };
}