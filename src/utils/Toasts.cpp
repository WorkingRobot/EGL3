#include "Toasts.h"

#include "Log.h"
#include "Version.h"

#include <glibmm.h>

namespace EGL3::Utils {
    using namespace WinToastLib;

    Toasts::Toasts() :
        Enabled(false),
        Impl(Utils::Version::GetAppName())
    {
        if (WinToast::IsCompatible()) {
            // This AUMI is set to the ProductGuid in packer_main.cpp
            // The ProductGUID is set to the AUMI in the shortcut in RegistryInfo.cpp
            auto Error = Impl.Initialize();
            if (EGL3_ENSUREF(Error == WinToastLib::Error::Success, LogLevel::Warning, "Toasts (notifications) have failed to initialize (Error: {})", (int)Error)) {
                Enabled = true;
            }
        }
    }

    Toasts::~Toasts()
    {

    }

    void Toasts::ShowToast(const ToastTemplate& Toast, const ToastHandler& Handler)
    {
        if (!Enabled) {
            return;
        }

        Glib::signal_idle().connect_once([this, Toast, Handler]() {
            Impl.ShowToast(Toast, Handler);
        }, Glib::PRIORITY_DEFAULT);
    }

    void Toasts::ClearToasts()
    {
        if (!Enabled) {
            return;
        }
        
        Glib::signal_idle().connect_once([this]() {
            Impl.ClearToasts();
        }, Glib::PRIORITY_DEFAULT);
    }

    bool Toasts::IsEnabled() const
    {
        return Enabled;
    }
}