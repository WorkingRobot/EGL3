#include "Toasts.h"

#include "Log.h"

#include <glibmm.h>

namespace EGL3::Utils {
    using namespace WinToastLib;

    Toasts::Toasts() :
        Enabled(false)
    {
        if (WinToast::isCompatible()) {
            // This AUMI is set to the ProductGuid in packer_main.cpp
            // The ProductGUID is set to the AUMI in the shortcut in RegistryInfo.cpp
            if (Impl.initialize(L"EGL3")) {
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
            Impl.showToast(Toast, Handler);
        }, Glib::PRIORITY_DEFAULT);
    }

    void Toasts::ClearToasts()
    {
        if (!Enabled) {
            return;
        }
        
        Glib::signal_idle().connect_once([this]() {
            Impl.clear();
        }, Glib::PRIORITY_DEFAULT);
    }

    bool Toasts::IsEnabled() const
    {
        return Enabled;
    }
}