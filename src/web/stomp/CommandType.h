#pragma once

#include "../../utils/Crc32.h"

namespace EGL3::Web::Stomp {
    enum class CommandType : uint8_t {
        Heartbeat,
        Connect,
        Connected,
        Send,
        Subscribe,
        Unsubscribe,
        Begin,
        Commit,
        Abort,
        Ack,
        Nack,
        Disconnect,
        Message,
        Reciept,
        Error
    };

    constexpr const char* CommandTypeToString(CommandType Type)
    {
        switch (Type)
        {
        case CommandType::Heartbeat:
            return "HEARTBEAT";
        case CommandType::Connect:
            return "CONNECT";
        case CommandType::Connected:
            return "CONNECTED";
        case CommandType::Send:
            return "SEND";
        case CommandType::Subscribe:
            return "SUBSCRIBE";
        case CommandType::Unsubscribe:
            return "UNSUBSCRIBE";
        case CommandType::Begin:
            return "BEGIN";
        case CommandType::Commit:
            return "COMMIT";
        case CommandType::Abort:
            return "ABORT";
        case CommandType::Ack:
            return "ACK";
        case CommandType::Nack:
            return "NACK";
        case CommandType::Disconnect:
            return "DISCONNECT";
        case CommandType::Message:
            return "MESSAGE";
        case CommandType::Reciept:
            return "RECIEPT";
        case CommandType::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
        }
    }

    constexpr CommandType StringToCommandType(std::string_view String)
    {
        switch (Utils::Crc32<true>(String.data(), String.size()))
        {
        case Utils::Crc32<true>("HEARTBEAT"):
            return CommandType::Heartbeat;
        case Utils::Crc32<true>("CONNECT"):
            return CommandType::Connect;
        case Utils::Crc32<true>("CONNECTED"):
            return CommandType::Connected;
        case Utils::Crc32<true>("SEND"):
            return CommandType::Send;
        case Utils::Crc32<true>("SUBSCRIBE"):
            return CommandType::Subscribe;
        case Utils::Crc32<true>("UNSUBSCRIBE"):
            return CommandType::Unsubscribe;
        case Utils::Crc32<true>("BEGIN"):
            return CommandType::Begin;
        case Utils::Crc32<true>("COMMIT"):
            return CommandType::Commit;
        case Utils::Crc32<true>("ABORT"):
            return CommandType::Abort;
        case Utils::Crc32<true>("ACK"):
            return CommandType::Ack;
        case Utils::Crc32<true>("NACK"):
            return CommandType::Nack;
        case Utils::Crc32<true>("DISCONNECT"):
            return CommandType::Disconnect;
        case Utils::Crc32<true>("MESSAGE"):
            return CommandType::Message;
        case Utils::Crc32<true>("RECIEPT"):
            return CommandType::Reciept;
        case Utils::Crc32<true>("ERROR"):
            return CommandType::Error;
        default:
            return (CommandType)-1;
        }
    }
}