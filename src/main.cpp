#include "modules/ModuleList.h"
#include "srv/pipe/Client.h"
#include "storage/game/Archive.h"
#include "storage/game/ArchiveList.h"
#include "storage/persistent/Store.h"
#include "utils/Assert.h"
#include "utils/Config.h"
#include "utils/Format.h"
#include "utils/GladeBuilder.h"
#include "utils/Platform.h"
#include "utils/Random.h"
#include "utils/FontSetup.h"

#include "storage/models/MountedDisk.h"
#include <future>

#include <fontconfig/fontconfig.h>
#include <gtkmm.h>
#include <pango/pangofc-fontmap.h>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
void* operator new(std::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
#else
#define ZoneNamedN(a, b, c) (void)0
#endif

namespace EGL3 {
    __forceinline int Start() {
        std::filesystem::current_path(Utils::Config::GetExeFolder());

        EGL3_LOG(LogLevel::Info, Utils::Format("Starting up %s/%s %s/%s", Utils::Config::GetAppName(), Utils::Config::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion().c_str()).c_str());

        EGL3_CONDITIONAL_LOG(!Utils::Config::GetConfigFolder().empty(), LogLevel::Critical, "Could not get config folder path");
        std::filesystem::create_directories(Utils::Config::GetConfigFolder());

        Utils::SetupFonts();

        //_putenv_s("GTK_CSD", "0");
        //_putenv_s("GTK_DEBUG", "interactive");

        auto App = Gtk::Application::create("me.workingrobot.egl3");

        Gtk::Settings::get_default()->set_property("gtk-application-prefer-dark-theme", true);

        auto StyleData = Gtk::CssProvider::create();
        StyleData->signal_parsing_error().connect([&](const Glib::RefPtr<const Gtk::CssSection>& section, const Glib::Error& error) {
            EGL3_LOG(LogLevel::Critical, "Failed to parse style data properly");
        });
        StyleData->load_from_path("resources\\EGL3.css");
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), StyleData, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        Utils::GladeBuilder Builder("resources\\EGL3.glade");

        auto& AppWnd = Builder.GetWidget<Gtk::ApplicationWindow>("EGL3App");

        App->signal_activate().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
            AppWnd.set_application(App);

            if (AppWnd.get_window()) {
                // This raise function is what allows the window to show and raise when the user tries to run EGL3 twice
                // When the second instance starts running, the activate signal here starts running, and since the window
                // hadn't initialized until after the first activate signal, this only runs in consecutive calls
                AppWnd.present();
            }
        }, App));

        Storage::Persistent::Store Storage(Utils::Config::GetConfigFolder() / "storage.stor");
        App->set_data("EGL3Storage", &Storage);

        Modules::ModuleList::Attach(App, Builder);
        return App->run(AppWnd, 0, NULL);
    }
}

#ifdef USE_SUBSYSTEM_CONSOLE

int main(int argc, char* argv[]) {
    return EGL3::Start();
}

#elif USE_SUBSYSTEM_WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    return EGL3::Start();
}

#endif
