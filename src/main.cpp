
#include "modules/ModuleList.h"
#include "storage/game/Archive.h"
#include "storage/game/ArchiveList.h"
#include "storage/persistent/Store.h"
#include "utils/Assert.h"
#include "utils/Config.h"
#include "utils/Format.h"
#include "utils/GladeBuilder.h"
#include "utils/Platform.h"

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
    int Start() {
        EGL3_LOG(LogLevel::Info, Utils::Format("Starting up %s/%s %s/%s", Utils::Config::GetAppName(), Utils::Config::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion().c_str()).c_str());

        constexpr static uint8_t TestBits = 21;

        Storage::Game::File TestFile{
            .Filename = {},
            .FileSize = 40592,
            .ChunkPartDataStartIdx = 394,
            .ChunkPartDataSize = 30
        };
        strcpy(TestFile.Filename, "Engine/Binaries/ThirdParty/CEF3/Win64/chrome_elf.dll");

        {
            ZoneNamedN(ZoneCreate, "Creating", true);
            Storage::Game::Archive Arch("test.egia", Storage::Game::ArchiveOptionCreate);
            Storage::Game::ArchiveList<Storage::Game::RunlistId::File> List(Arch);
            sizeof(Storage::Game::File);
            List.resize(1 << TestBits);

            {
                ZoneNamedN(ZoneWrite, "Writing", true);
                std::fill(List.begin(), List.end(), TestFile);
            }
        }

        uint64_t V;
        {
            ZoneNamedN(ZoneRead, "Reading", true);
            Storage::Game::Archive Arch("test.egia", Storage::Game::ArchiveOptionRead);
            Storage::Game::ArchiveList<Storage::Game::RunlistId::File> List(Arch);

            {
                ZoneNamedN(ZoneAccumulate, "Accumulating", true);
                V = std::accumulate(List.begin(), List.end(), 0ull, [](uint64_t A, const Storage::Game::File& B) {
                    return A + B.FileSize;
                });
            }
        }

        printf("%llu\n", V);

        return 0;

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

        App->set_data("EGL3Storage", new Storage::Persistent::Store("storage.stor"), [](void* Store) {
            delete (Storage::Persistent::Store*)Store;
        });

        App->signal_shutdown().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
            App->remove_data("EGL3Storage");
        }, App));

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
