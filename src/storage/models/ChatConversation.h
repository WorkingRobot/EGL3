#pragma once

#include "ChatMessage.h"

#include <deque>
#include <string>

namespace EGL3::Storage::Models {
    struct ChatConversation {
        // Account id of the user we're talking to
        std::string AccountId;
        // All the messages shared between us
        std::deque<ChatMessage> Messages;
    };
}