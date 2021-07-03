#pragma once

#include "../../utils/Crc32.h"
#include "../Hosts.h"

namespace EGL3::Web::Xmpp {
    enum class Status : uint8_t {
        Offline,
        ExtendedAway,
        Away,
        Online,
        DoNotDisturb,
        Chat
    };

    static constexpr const char* StatusToHumanString(Status Status) {
        switch (Status)
        {
        case Status::DoNotDisturb:
            return "Do Not Disturb";
        case Status::Chat:
            return "Chat";
        case Status::Online:
            return "Online";
        case Status::Away:
            return "Away";
        case Status::ExtendedAway:
            return "Extended Away";
        case Status::Offline:
            return "Offline";
        default:
            return "Unknown";
        }
    }

    static constexpr const char* StatusToString(Status Status) {
        switch (Status)
        {
        case Status::DoNotDisturb:
            return "dnd";
        case Status::Chat:
            return "chat";
        case Status::Online:
            return "online";
        case Status::Away:
            return "away";
        case Status::ExtendedAway:
            return "xa";
        case Status::Offline:
            return "offline";
        default:
            return "unknown";
        }
    }

    static constexpr Status StringToStatus(std::string_view String) {
        switch (Utils::Crc32(String.data(), String.size()))
        {
        case Utils::Crc32("dnd"):
            return Status::DoNotDisturb;
        case Utils::Crc32("chat"):
            return Status::Chat;
        case Utils::Crc32("online"):
            return Status::Online;
        case Utils::Crc32("away"):
            return Status::Away;
        case Utils::Crc32("xa"):
            return Status::ExtendedAway;
        case Utils::Crc32("offline"):
            return Status::Offline;
        default:
            return Status::Online;
        }
    }

    static std::string StatusToUrl(Status Status) {
        switch (Status)
        {
        case Status::Chat:
            return std::format("{}status/chat.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Status::DoNotDisturb:
            return std::format("{}status/dnd.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Status::Online:
            return std::format("{}status/online.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Status::Away:
            return std::format("{}status/away.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Status::ExtendedAway:
            return std::format("{}status/xa.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Status::Offline:
        default:
            return std::format("{}status/offline.png", Web::GetHostUrl<Web::Host::EGL3>());
        }
    }
}
