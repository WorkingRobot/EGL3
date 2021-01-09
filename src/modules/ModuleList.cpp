#include "ModuleList.h"

#include "AsyncFF.h"
#include "Authorization.h"
#include "Game.h"
#include "ImageCache.h"
#include "StatsGraph.h"
#include "StatusPage.h"
#include "WhatsNew.h"

#include "Friends/Friends.h"
#include "Friends/Chat.h"
#include "Friends/List.h"
#include "Friends/Options.h"
#include "Friends/KairosMenu.h"

namespace EGL3::Modules {
	ModuleList::ModuleList(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder) {
		App->set_data("EGL3Modules", this, [](void* Data) {
			delete (ModuleList*)Data;
		});
		App->signal_shutdown().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
			App->remove_data("EGL3Modules");
		}, App));

		AddModules(App, Builder, *(Storage::Persistent::Store*)App->get_data("EGL3Storage"));
	}

	void ModuleList::AddModules(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder, Storage::Persistent::Store& Storage) {
		AddModule<AsyncFFModule>();
		AddModule<ImageCacheModule>();
		AddModule<StatsGraphModule>(Builder);
		AddModule<StatusPageModule>(Builder);
		AddModule<AuthorizationModule>(Storage, Builder);

		AddModule<WhatsNewModule>(*this, Storage, Builder);

		AddModule<Friends::OptionsModule>(*this, Storage, Builder);
		AddModule<Friends::KairosMenuModule>(*this, Builder);
		AddModule<Friends::ListModule>(*this, Builder);
		AddModule<Friends::ChatModule>(*this, Builder);
		AddModule<Friends::FriendsModule>(*this, Storage, Builder);

		// AddModule<GameModule>(Builder);
	}

	template<typename T, typename... Args>
	void ModuleList::AddModule(Args&&... ModuleArgs) {
		Modules.emplace_back(std::make_shared<T>(std::forward<Args>(ModuleArgs)...));
	}

	void ModuleList::Attach(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder) {
		new ModuleList(App, Builder);
	}
}