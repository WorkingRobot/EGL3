#include "ModuleList.h"

#include "AsyncFF.h"
#include "Authorization.h"
#include "ImageCache.h"
#include "StatsGraph.h"
#include "StatusPage.h"
#include "Taskbar.h"
#include "WhatsNew.h"

#include "Friends/Friends.h"
#include "Friends/Chat.h"
#include "Friends/List.h"
#include "Friends/Options.h"
#include "Friends/KairosMenu.h"

#include "Game/Game.h"
#include "Game/Download.h"
#include "Game/GameInfo.h"
#include "Game/Play.h"
#include "Game/Service.h"
#include "Game/UpdateCheck.h"

namespace EGL3::Modules {
    ModuleList::ModuleList(const std::filesystem::path& BuilderPath, const std::filesystem::path& StoragePath) :
        Builder(BuilderPath),
        Storage(StoragePath)
    {
        AddModules();
    }

    ModuleList::~ModuleList()
    {
        // Delete in reverse to perserve dependencies
        decltype(Modules.rbegin()) Itr;
        while ((Itr = Modules.rbegin()) != Modules.rend()) {
            Modules.erase(--(Itr.base()));
        }
    }

    const Utils::GladeBuilder& ModuleList::GetBuilder() const
    {
        return Builder;
    }

    const Storage::Persistent::Store& ModuleList::GetStorage() const
    {
        return Storage;
    }

    Storage::Persistent::Store& ModuleList::GetStorage()
    {
        return Storage;
    }

    void ModuleList::AddModules() {
        AddModule<AsyncFFModule>();
        AddModule<ImageCacheModule>();
        AddModule<TaskbarModule>(Builder);
        AddModule<StatsGraphModule>(Builder);
        AddModule<StatusPageModule>(Builder);
        AddModule<AuthorizationModule>(Storage);

        AddModule<WhatsNewModule>(*this, Storage, Builder);

        AddModule<Friends::OptionsModule>(*this, Storage, Builder);
        AddModule<Friends::ListModule>(*this, Builder);
        AddModule<Friends::KairosMenuModule>(*this, Builder);
        AddModule<Friends::ChatModule>(*this, Builder);
        AddModule<Friends::FriendsModule>(*this, Storage, Builder);

        AddModule<Game::ServiceModule>();
        AddModule<Game::GameInfoModule>(*this);
        AddModule<Game::DownloadModule>(*this, Storage, Builder);
        AddModule<Game::PlayModule>(*this, Storage, Builder);
        AddModule<Game::UpdateCheckModule>(*this, Storage);
        AddModule<Game::GameModule>(*this, Storage, Builder);
    }

    template<typename T, typename... Args>
    void ModuleList::AddModule(Args&&... ModuleArgs) {
        Modules.emplace_back(std::make_shared<T>(std::forward<Args>(ModuleArgs)...));
    }
}