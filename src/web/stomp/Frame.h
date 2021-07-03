#pragma once

#include "CommandType.h"

#include <ixwebsocket/IXWebSocketHttpHeaders.h>
#include <memory>
#include <string>

namespace EGL3::Web::Stomp {
    using Headers = ix::WebSocketHttpHeaders;

    class Frame {
    public:
        Frame(CommandType Command, Headers&& Headers, std::string&& Body);

        Frame(std::string_view Data);

        std::string Encode() const;

        CommandType GetCommand() const noexcept;

        const Headers& GetHeaders() const noexcept;

        const std::string& GetBody() const noexcept;

    private:
        void Decode(std::string_view Data);

        CommandType Command;
        Headers Headers;
        std::string Body;
    };
}