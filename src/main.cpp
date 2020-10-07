#include <gtkmm.h>

#include "web/epic/auth/DeviceCode.h"
#include "utils/OpenBrowser.h"
#include "modules/Modules.h"
#include "utils/GladeBuilder.h"
#include "web/epic/EpicClient.h"
#include "web/epic/responses/Responses.h"
#include "storage/persistent/Store.h"
#include "utils/mmio/MmioFile.h"

namespace EGL3 {
    __forceinline int Start() {
        Utils::Mmio::MmioReadonlyFile n(fs::path("test.dat"));

        auto v = n.Get();

        static const cpr::Authentication AuthClientSwitch{ "5229dcd3ac3845208b496649092f251b", "e3bd2d3e-bf8c-4857-9e7d-f3d947d220c7" };
        static const cpr::Authentication AuthClientLauncher{ "34a02cf8f4414e29b15921876da36f9a", "daafbccc737745039dffe53d94fc76cf" };

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

        if (!getenv("GTK_CSD")) {
            _putenv_s("GTK_CSD", "0");
        }
        _putenv_s("GTK_DEBUG", "interactive");
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

        App->set_data("EGL3Storage", new Storage::Persistent::Store("storage.stor"), [](void* Store) {
            delete (Storage::Persistent::Store*)Store;
        });

        App->signal_activate().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
            AppWnd.set_application(App);

            if (AppWnd.get_window()) {
                // This raise function is what allows the window to show and raise when the user tries to run EGL3 twice
                // When the second instance starts running, the activate signal here starts running, and since the window
                // hadn't initialized until after the first activate signal, this only runs in consecutive calls
                AppWnd.present();
            }
        }, App));

        App->signal_startup().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
            Modules::Initialize(App, Builder);
        }, App));

        App->signal_shutdown().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
            Modules::Destroy(App);
            App->remove_data("EGL3Storage");
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
