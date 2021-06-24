#pragma once

#include "Log.h"

#include <gtkmm.h>

namespace EGL3::Utils {
    struct SlotHolder {
        SlotHolder() = default;

        SlotHolder(const SlotHolder&) = delete;
        SlotHolder& operator=(const SlotHolder&) = delete;

        SlotHolder(SlotHolder&&) = delete;
        SlotHolder& operator=(SlotHolder&&) = delete;

        template<class... ArgsT>
        SlotHolder& operator=(ArgsT&&... Args) {
            EGL3_VERIFY(!Connected, "Slot is already connected");
            Connection = sigc::connection(std::forward<ArgsT>(Args)...);
            Connected = true;

            return *this;
        }


        template<class... ArgsT>
        SlotHolder(ArgsT&&... Args) :
            Connection(std::forward<ArgsT>(Args)...),
            Connected(true)
        {
            
        }

        ~SlotHolder() {
            if (Connected) {
                Connection.disconnect();
            }
        }

    private:
        bool Connected = false;
        sigc::connection Connection;
    };

    struct SharedSlotHolder {
        SharedSlotHolder() = default;

        template<class... ArgsT>
        SharedSlotHolder(ArgsT&&... Args) :
            Connection(std::make_shared<SlotHolder>(std::forward<ArgsT>(Args)...))
        {

        }

    private:
        std::shared_ptr<SlotHolder> Connection;
    };
}