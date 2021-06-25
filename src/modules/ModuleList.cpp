#include "ModuleList.h"

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
        Storage(StoragePath),
        AuthedModulesIdx(0)
    {
        AddModulesCore();

        LoggedInDispatcher.connect([this]() {
            AddModulesLoggedIn();
        });
        auto& Auth = GetModule<Login::AuthModule>();
        Auth.LoggedIn.connect([this]() {
            LoggedInDispatcher.emit();
        });
        Auth.LoggedOut.connect([this]() {
            RemoveModulesLoggedIn();
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

    bool ModuleList::DisplayConfirmation(const Glib::ustring& Message, const Glib::ustring& Title, bool UseMarkup) const
    {
        Gtk::MessageDialog Dialog(GetWidget<Gtk::Window>("EGL3App"), Message, UseMarkup, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        Dialog.set_title(Title);
        Dialog.set_size_request(300, 200);
        return Dialog.run() == Gtk::RESPONSE_YES;
    }

    bool ModuleList::DisplayConfirmation(const Glib::ustring& Message, const Glib::ustring& SecondaryMessage, const Glib::ustring& Title, bool UseMarkup) const
    {
        Gtk::MessageDialog Dialog(GetWidget<Gtk::Window>("EGL3App"), Message, UseMarkup, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        Dialog.set_title(Title);
        Dialog.set_secondary_text(SecondaryMessage, UseMarkup);
        Dialog.set_size_request(300, 200);
        return Dialog.run() == Gtk::RESPONSE_YES;
    }

    void ModuleList::DisplayError(const Glib::ustring& Message, const Glib::ustring& Title, bool UseMarkup) const
    {
        Gtk::MessageDialog Dialog(GetWidget<Gtk::Window>("EGL3App"), Message, UseMarkup, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        Dialog.set_title(Title);
        Dialog.set_size_request(300, 200);
        Dialog.run();
    }

    void ModuleList::DisplayError(const Glib::ustring& Message, const Glib::ustring& SecondaryMessage, const Glib::ustring& Title, bool UseMarkup) const
    {
        Gtk::MessageDialog Dialog(GetWidget<Gtk::Window>("EGL3App"), Message, UseMarkup, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        Dialog.set_title(Title);
        Dialog.set_secondary_text(SecondaryMessage, UseMarkup);
        Dialog.set_size_request(300, 200);
        Dialog.run();
    }

    void ModuleList::AddModulesCore()
    {
        AddModule<AsyncFFModule>();
        AddModule<ImageCacheModule>();
        AddModule<TaskbarModule>();

        AddModule<Login::StackModule>();
        AddModule<Login::AuthModule>();
        AddModule<Login::HeaderModule>();
        AddModule<Login::ChooserModule>();

        AddModule<Game::ServiceModule>();
    }

    void ModuleList::AddModulesLoggedIn()
    {
        EGL3_VERIFY(AuthedModulesIdx == 0, "Attempted to log in twice without logging out first");

        AuthedModulesIdx = Modules.size();

        AddModule<StatsGraphModule>();
        AddModule<StatusPageModule>();
        AddModule<WhatsNewModule>();

        AddModule<Friends::OptionsModule>();
        AddModule<Friends::ListModule>();
        AddModule<Friends::KairosMenuModule>();
        AddModule<Friends::ChatModule>();
        AddModule<Friends::FriendsModule>();

        AddModule<Game::GameInfoModule>();
        AddModule<Game::DownloadModule>();
        AddModule<Game::PlayModule>();
        AddModule<Game::UpdateCheckModule>();
        AddModule<Game::GameModule>();
    }

    void ModuleList::RemoveModulesLoggedIn()
    {
        EGL3_VERIFY(AuthedModulesIdx != 0, "Attempted to log out without logging in first");
        // Delete in reverse to perserve dependencies
        // std::vector doesn't guarantee reverse destruction like std::array does
        decltype(Modules.rbegin()) Itr;
        while ((Itr = Modules.rbegin()) != Modules.rend() - AuthedModulesIdx) {
            Modules.erase(--(Itr.base()));
        }
        AuthedModulesIdx = 0;
    }

    template<typename T>
    void ModuleList::AddModule() {
        Modules.emplace_back(std::make_unique<T>(*this));
        EGL3_LOGF(LogLevel::Info, "{} loaded", Detail::module_name_v<T>);
    }
}