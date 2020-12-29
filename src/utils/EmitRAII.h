#pragma once

#include <gtkmm.h>

namespace EGL3::Utils {
    struct EmitRAII {
        EmitRAII(Glib::Dispatcher& Dispatcher) :
            Dispatcher(Dispatcher)
        {

        }

        ~EmitRAII() {
            Dispatcher.emit();
        }

    private:
        Glib::Dispatcher& Dispatcher;
    };
}