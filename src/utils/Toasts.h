#pragma once

#include "Callback.h"

#include <wintoastlib.h>

namespace EGL3::Utils {
    using ToastTemplate = WinToastLib::WinToastTemplate;
    using ToastType = WinToastLib::WinToastTemplateType;
    using ToastDismissalReason = WinToastLib::WinToastDismissalReason;
    using ToastHandler = WinToastLib::WinToastHandler;

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