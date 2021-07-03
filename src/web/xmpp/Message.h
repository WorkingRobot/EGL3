#pragma once

#include "UserJid.h"

namespace EGL3::Web::Xmpp {
    struct Message {
        UserJid From;
        UserJid To;
        std::string Payload;
    };
}