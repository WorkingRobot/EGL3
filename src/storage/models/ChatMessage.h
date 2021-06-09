#pragma once

#include "../../web/JsonParsing.h"

namespace EGL3::Storage::Models {
    struct ChatMessage {
        // Actual text data
        std::string Content;
        // Timestamp of the message
        Web::TimePoint Time;
        // True if remote (they sent it), false if local (we sent it)
        bool Recieved;
    };
}