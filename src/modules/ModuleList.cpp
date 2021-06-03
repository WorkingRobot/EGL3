#include "ModuleList.h"

#include "../utils/type_name.h"

#include "AsyncFF.h"
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

#include "Login/Header.h"
#include "Login/Chooser.h"
#include "Login/Stack.h"
#include "Login/Auth.h"

namespace EGL3::Modules {
    ModuleList::ModuleList(const std::filesystem::path& BuilderPath, const std::filesystem::path& StoragePath) :
        Builder(BuilderPath),
        Storage(StoragePath)
    {
        AddModulesCore();

        LoggedInDispatcher.connect([this]() {
            AddModulesLoggedIn();
        });
        GetModule<Login::AuthModule>().LoggedIn.connect([this]() {
            LoggedInDispatcher.emit();
        });
    }

    ModuleList::~ModuleList()
    {
        // Delete in reverse to perserve dependencies
        // std::vector doesn't guarantee reverse destruction like std::array does
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

    void ModuleList::AddModulesCore()
    {
        AddModule<AsyncFFModule>();
        AddModule<ImageCacheModule>();
        AddModule<TaskbarModule>();

        AddModule<Login::HeaderModule>();
        AddModule<Login::ChooserModule>();
        AddModule<Login::StackModule>();
        AddModule<Login::AuthModule>();
    }

    void ModuleList::AddModulesLoggedIn()
    {
        AddModule<StatsGraphModule>();
        AddModule<StatusPageModule>();
        AddModule<WhatsNewModule>();

        AddModule<Friends::OptionsModule>();
        AddModule<Friends::ListModule>();
        AddModule<Friends::KairosMenuModule>();
        AddModule<Friends::ChatModule>();
        AddModule<Friends::FriendsModule>();

        AddModule<Game::ServiceModule>();
        AddModule<Game::GameInfoModule>();
        AddModule<Game::DownloadModule>();
        AddModule<Game::PlayModule>();
        AddModule<Game::UpdateCheckModule>();
        AddModule<Game::GameModule>();
    }

    template<typename T>
    void ModuleList::AddModule() {
        Modules.emplace_back(std::make_unique<T>(*this));
        static constexpr std::string_view Name = type_name_v<T>.substr(sizeof("class EGL3::Modules::") - 1);
        EGL3_LOG(LogLevel::Info, Utils::Format("%.*s loaded", Name.size(), Name.data()).c_str());
    }
}