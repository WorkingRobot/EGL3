#pragma once

#include "Callback.h"

#include <wintoastlib.h>

namespace EGL3::Utils {
    using ToastTemplate = WinToastLib::Template;
    using ToastType = WinToastLib::TemplateType;
    using ToastDismissalReason = WinToastLib::DismissalReason;
    using ToastHandler = WinToastLib::Handler;

    class Toasts {
    public:
        Toasts();

        ~Toasts();

        void ShowToast(const ToastTemplate& Toast, const ToastHandler& Handler);

        void ClearToasts();

        bool IsEnabled() const;

    private:
        bool Enabled;
        WinToastLib::WinToast Impl;
    };
}