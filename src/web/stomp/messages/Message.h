#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Stomp::Messages {
    struct Message {
        std::string CorrelationId;
        TimePoint Timestamp;
        std::optional<std::string> Id;
        std::string ConnectionId;
        std::optional<JsonObject> Payload;
        std::string Type;

        PARSE_DEFINE(Message)
            PARSE_ITEM("correlationId", CorrelationId)
            PARSE_ITEM("timestamp", Timestamp)
            PARSE_ITEM_OPT("id", Id)
            PARSE_ITEM("connectionId", ConnectionId)
            PARSE_ITEM_OPT("payload", Payload)
            PARSE_ITEM("type", Type)
        PARSE_END
    };
}
