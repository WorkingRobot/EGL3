#pragma once

#include <chrono>
#include <string>

namespace EGL3::Storage::Models {
    struct ChatMessage {
        // Actual text data
        std::string Content;
        // Timestamp of the message
        std::chrono::utc_clock::time_point Time;
        // True if remote (they sent it), false if local (we sent it)
        bool Recieved;
    };
}