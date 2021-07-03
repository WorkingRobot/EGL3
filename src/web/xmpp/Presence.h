#pragma once

#include "../JsonParsing.h"
#include "Status.h"

namespace EGL3::Web::Xmpp {
    struct Presence {
        bool Available;
        Status Status;
        std::string StatusText;
        TimePoint Timestamp;
    };
}