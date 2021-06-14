#include "modules/ModuleList.h"
#include "modules/Login/Auth.h"
#include "utils/Log.h"
#include "utils/Config.h"
#include "utils/FontSetup.h"
#include "utils/Platform.h"

namespace EGL3 {
    class MainApp {
    public:
        MainApp() :
            App(Gtk::Application::create("me.workingrobot.egl3", Gio::ApplicationFlags::APPLICATION_HANDLES_COMMAND_LINE))
        {
            EGL3_LOGF(LogLevel::Info, "Starting up {}/{} {}/{}", Utils::Config::GetAppName(), Utils::Config::GetAppVersion(), Utils::Platform::GetOSName(), Utils::Platform::GetOSVersion());
            EGL3_ENSUREF(!Utils::Platform::IsProcessElevated(), LogLevel::Warning, "EGL3 is running as admin. Any files created (config files, install files, etc.) will be readonly when run without admin in the future.");

            App->signal_command_line().connect(sigc::mem_fun(this, &MainApp::OnCommandLine), false);
            App->signal_startup().connect(sigc::mem_fun(this, &MainApp::OnStartup));
            App->signal_activate().connect(sigc::mem_fun(this, &MainApp::OnActivate));
        }

        int Run() {
            return App->run(__argc, __argv);
        }

    private:
        void OnStartup() {
            std::filesystem::current_path(Utils::Config::GetExeFolder());

            Utils::Config::SetupFolders();

            Utils::SetupFonts();

            {
                auto Settings = Gtk::Settings::get_default();
                Settings->property_gtk_application_prefer_dark_theme().set_value(true);
                Settings->property_gtk_xft_dpi().set_value(128 * 1024); // 128 dpi (125%)
            }

            {
                auto StyleData = Gtk::CssProvider::create();
                StyleData->signal_parsing_error().connect([&](const Glib::RefPtr<const Gtk::CssSection>& Section, const Glib::Error& Error) {
                    EGL3_ABORTF("Failed to parse style data properly ({}) at {} @ {}", (std::string)Error.what(), Section->get_file()->get_path(), Section->get_start_line() + 1);
                });
                StyleData->load_from_path("resources\\EGL3.css");
                Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), StyleData, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            }

            PrimaryData = std::make_unique<Modules::ModuleList>("resources\\EGL3.glade", Utils::Config::GetFolder() / "storage.stor");
        }

        void OnActivate() {
            auto& AppWnd = PrimaryData->GetBuilder().GetWidget<Gtk::ApplicationWindow>("EGL3App");

            App->add_window(AppWnd);

            AppWnd.present();
        }

        int OnCommandLine(const Glib::RefPtr<Gio::ApplicationCommandLine>& CommandLine) {
            App->activate();

            PrimaryData->GetModule<Modules::Login::AuthModule>().HandleCommandLine(CommandLine);

            return 0;
        }

        Glib::RefPtr<Gtk::Application> App;
        std::unique_ptr<Modules::ModuleList> PrimaryData;
    };

    __forceinline int Start() {
        //_putenv_s("GTK_DEBUG", "interactive");
        MainApp App;

        return App.Run();
    }
}

#ifdef USE_SUBSYSTEM_CONSOLE

int main(int argc, char* argv[]) {
    return EGL3::Start();
}

#elif USE_SUBSYSTEM_WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    return EGL3::Start();
}

#endif
