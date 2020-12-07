#include <gtkmm.h>

#include "web/epic/auth/ClientCredentials.h"
#include "web/epic/auth/DeviceCode.h"
#include "utils/OpenBrowser.h"
#include "modules/ModuleList.h"
#include "utils/GladeBuilder.h"
#include "web/epic/EpicClientAuthed.h"
#include "web/xmpp/XmppClient.h"
#include "storage/persistent/Store.h"
#include "utils/mmio/MmioFile.h"
#include <Exports/UObject.h>

#include <fontconfig/fontconfig.h>
#include <pango/pangofc-fontmap.h>

namespace EGL3 {
    __forceinline int Start() {
        static const cpr::Authentication AuthClientSwitch{ "5229dcd3ac3845208b496649092f251b", "e3bd2d3e-bf8c-4857-9e7d-f3d947d220c7" };
        static const cpr::Authentication AuthClientLauncher{ "34a02cf8f4414e29b15921876da36f9a", "daafbccc737745039dffe53d94fc76cf" };
        static const cpr::Authentication AuthClientAndroidPortal{ "38dbfc3196024d5980386a37b7c792bb", "a6280b87-e45e-409b-9681-8f15eb7dbcf5" };

        if constexpr (false)
        {
            Utils::Mmio::MmioReadonlyFile n(fs::path("test.dat"));

            auto v = n.Get();
        }

        if constexpr (false)
        {
            Web::Epic::Auth::ClientCredentials CredsAuth(AuthClientAndroidPortal);
            EGL3_ASSERT(CredsAuth.GetOAuthResponseFuture().get() == Web::Epic::Auth::ClientCredentials::SUCCESS, "Could not get oauth data");
            Web::Epic::EpicClientAuthed C(CredsAuth.GetOAuthResponse(), AuthClientAndroidPortal);

            auto Resp = C.GetDownloadInfo("Windows", "Live", "4fe75bbc5a674f4f9b356b5c90567da5", "Fortnite");
            EGL3_ASSERT(!Resp.HasError(), "Could not get download info");
            auto& El = Resp->Elements;
        }

        /*Web::Epic::Auth::DeviceCode DevCodeAuth(AuthClientSwitch);
        auto BrowserUrlResult = DevCodeAuth.GetBrowserUrlFuture().get();
        EGL3_ASSERT(BrowserUrlResult == Web::Epic::Auth::DeviceCode::ERROR_SUCCESS, "Could not get browser url");
        Utils::OpenInBrowser(DevCodeAuth.GetBrowserUrl());

        auto OAuthRespResult = DevCodeAuth.GetOAuthResponseFuture().get();
        EGL3_ASSERT(OAuthRespResult == Web::Epic::Auth::DeviceCode::ERROR_SUCCESS, "Could not get oauth data");

        Web::Epic::Responses::OAuthToken AuthData;
        EGL3_ASSERT(Web::Epic::Responses::OAuthToken::Parse(DevCodeAuth.GetOAuthResponse(), AuthData), "Could not parse oauth data");

        Storage::Persistent::Store St("storage.stor");
        auto& ret = St.Get(Storage::Persistent::Key::Hello);

        Web::Epic::EpicClient C;
        auto Resp = C.GetBlogPosts("en-US");*/

        /*if (!getenv("GTK_CSD")) {
            _putenv_s("GTK_CSD", "0");
        }*/

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
        //_putenv_s("GTK_DEBUG", "interactive");
        //_putenv_s("GOBJECT_DEBUG", "instance-count");

        auto App = Gtk::Application::create("me.workingrobot.egl3");

        Gtk::Settings::get_default()->set_property("gtk-application-prefer-dark-theme", true);

        auto StyleData = Gtk::CssProvider::create();
        StyleData->signal_parsing_error().connect([&](const Glib::RefPtr<const Gtk::CssSection>& section, const Glib::Error& error) {
            printf("error oops\n");
        });
        StyleData->load_from_path("J:/Code/Visual Studio 2017/Projects/EGL3/src/EGL3.css");

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

        App->signal_startup().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
            try {
                Modules::ModuleList::Attach(App, Builder);
            }
            catch (std::exception& e) {
                printf("%s\n", e.what());
            }
        }, App));

        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), StyleData, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
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
