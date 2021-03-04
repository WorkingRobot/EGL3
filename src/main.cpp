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
        EGL3_LOG(LogLevel::Info, Utils::Format("Starting up %s/%s %s/%s", Utils::Config::GetAppName(), Utils::Config::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion().c_str()).c_str());

        if constexpr (false)
        {
            Service::Pipe::Client Client;
            void* Ctx = nullptr;
            EGL3_CONDITIONAL_LOG(Client.OpenArchive(R"(J:\Code\Visual Studio 2017\Projects\EGL3\out\build\x64-Release\Fortnite.egia)", Ctx), LogLevel::Critical, "Could not OpenArchive");
            EGL3_CONDITIONAL_LOG(Client.ReadArchive(Ctx), LogLevel::Critical, "Could not ReadArchive");
            EGL3_CONDITIONAL_LOG(Client.InitializeDisk(Ctx), LogLevel::Critical, "Could not InitializeDisk");
            EGL3_CONDITIONAL_LOG(Client.CreateLUT(Ctx), LogLevel::Critical, "Could not CreateLUT");
            EGL3_CONDITIONAL_LOG(Client.CreateDisk(Ctx), LogLevel::Critical, "Could not CreateDisk");
            EGL3_CONDITIONAL_LOG(Client.MountDisk(Ctx), LogLevel::Critical, "Could not MountDisk");
            printf("dun!\n");
            return 0;
        }

        if constexpr(false)
        {
            std::vector<Storage::Models::MountedFile> Files{
                { "name/name2.exe", 818304, fopen(R"(C:\Users\Aleks\Desktop\EasyAntiCheat_Setup.exe)", "rb") },
            };

            Storage::Models::MountedDisk Disk(Files, Utils::Random());
            Disk.HandleFileCluster.Set([](void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) {
                auto fp = (FILE*)Ctx;
                fseek(fp, LCN * 4096, SEEK_SET);
                fread(Buffer, 1, 4096, fp);
            });
            Disk.Initialize();
            Disk.Create();
            Disk.Mount();
            std::this_thread::sleep_for(std::chrono::hours(24));
            return 0;
        }

        {
            auto Config = FcConfigCreate();
            FcConfigSetCurrent(Config);
            FcConfigParseAndLoad(Config, (FcChar8*)R"(J:\Code\Visual Studio 2017\Projects\EGL3\src\graphics\fonts\fonts.conf)", true);
            FcConfigAppFontAddDir(Config, (FcChar8*)R"(J:\Code\Visual Studio 2017\Projects\EGL3\src\graphics\fonts)");
            FcConfigAppFontAddDir(Config, (FcChar8*)R"(C:\WINDOWS\Fonts)");

            auto FontMap = (PangoFcFontMap*)pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
            pango_fc_font_map_set_config(FontMap, Config);
            pango_cairo_font_map_set_default((PangoCairoFontMap*)FontMap);
        }

        //_putenv_s("GTK_CSD", "0");
        //_putenv_s("GTK_DEBUG", "interactive");

        auto App = Gtk::Application::create("me.workingrobot.egl3");

        Gtk::Settings::get_default()->set_property("gtk-application-prefer-dark-theme", true);

        auto StyleData = Gtk::CssProvider::create();
        StyleData->signal_parsing_error().connect([&](const Glib::RefPtr<const Gtk::CssSection>& section, const Glib::Error& error) {
            EGL3_LOG(LogLevel::Critical, "Failed to parse style data properly");
        });
        StyleData->load_from_path("J:/Code/Visual Studio 2017/Projects/EGL3/src/EGL3.css");
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), StyleData, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        Utils::GladeBuilder Builder("J:/Code/Visual Studio 2017/Projects/EGL3/src/EGL3.glade");

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

        Storage::Persistent::Store Storage("storage.stor");
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
