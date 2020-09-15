#include <gtkmm.h>

#include "modules/Modules.h"
#include "utils/GladeBuilder.h"

__forceinline int start() {
    if (!getenv("GTK_CSD")) {
        _putenv_s("GTK_CSD", "0");
    }

    auto App = Gtk::Application::create("me.workingrobot.egl3");

    GladeBuilder Builder("J:/Code/Visual Studio 2017/Projects/EGL3/src/EGL3.glade");

    auto& AppWnd = Builder.GetWidget<Gtk::ApplicationWindow>("EGL3App");

    InitializeModules(AppWnd, Builder);

    App->signal_activate().connect(sigc::bind([&](const Glib::RefPtr<Gtk::Application>& App) {
        AppWnd.set_application(App);

        if (AppWnd.get_window()) {
            // This raise function is what allows the window to show and raise when the user tries to run EGL3 twice
            // When the second instance starts running, the activate signal here starts running, and since the window
            // hadn't initialized until after the first activate signal, this only runs in consecutive calls
            AppWnd.present();
        }
    }, App));

    return App->run(AppWnd, 0, NULL);
}

#ifdef USE_SUBSYSTEM_CONSOLE

int main(int argc, char* argv[]) {
    return start();
}

#elif USE_SUBSYSTEM_WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    return start();
}

#endif
