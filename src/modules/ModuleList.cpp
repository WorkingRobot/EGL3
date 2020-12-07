#include "ModuleList.h"

#include "Authorization.h"
#include "Friends.h"
#include "Game.h"
#include "ImageCache.h"
#include "SidebarNotebook.h"
#include "StatsGraph.h"
#include "StatusPage.h"
#include "WhatsNew.h"

namespace EGL3::Modules {
	void ModuleList::AddModules(const Glib::RefPtr<Gtk::Application>& App, const Utils::GladeBuilder& Builder, Storage::Persistent::Store& Storage) {
		AddModule<ImageCacheModule>();
		//AddModule<SidebarNotebookModule>(Builder);
		AddModule<WhatsNewModule>(*this, Storage, Builder);
		AddModule<StatsGraphModule>(Builder);
		AddModule<StatusPageModule>(Builder);
		AddModule<AuthorizationModule>(Storage, Builder);
		AddModule<FriendsModule>(*this, Builder);
		// AddModule<GameModule>(Builder);
	}
}