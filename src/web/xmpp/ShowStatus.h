#pragma once

#include "../../utils/Format.h"
#include "../../web/Hosts.h"

namespace EGL3::Web::Xmpp::Json {
    // There's a bunch of enums that this is based off of,
    // but the most high level one is EOnlinePresenceState
    // https://github.com/EpicGames/UnrealEngine/blob/f8f4b403eb682ffc055613c7caf9d2ba5df7f319/Engine/Source/Runtime/Online/XMPP/Private/XmppJingle/XmppPresenceJingle.cpp#L170
    enum class ShowStatus : uint8_t {
        Offline,
        ExtendedAway, // xa
        Away, // away
        Online,
        DoNotDisturb, // dnd
        Chat, // chat
    };

    static constexpr const char* ShowStatusToString(ShowStatus Status) {
        switch (Status)
        {
        case ShowStatus::DoNotDisturb:
            return "Do Not Disturb";
        case ShowStatus::Chat:
            return "Chat";
        case ShowStatus::Online:
            return "Online";
        case ShowStatus::Away:
            return "Away";
        case ShowStatus::ExtendedAway:
            return "Extended Away";
        case ShowStatus::Offline:
            return "Offline";
        default:
            return "Unknown";
        }
    }

    static std::string ShowStatusToUrl(ShowStatus Status) {
        switch (Status)
        {
        case ShowStatus::Chat:
            return Utils::Format("%sstatus/chat.png", Web::GetHostUrl<Web::Host::EGL3>());
        case ShowStatus::DoNotDisturb:
            return Utils::Format("%sstatus/dnd.png", Web::GetHostUrl<Web::Host::EGL3>());
        case ShowStatus::Online:
            return Utils::Format("%sstatus/online.png", Web::GetHostUrl<Web::Host::EGL3>());
        case ShowStatus::Away:
            return Utils::Format("%sstatus/away.png", Web::GetHostUrl<Web::Host::EGL3>());
        case ShowStatus::ExtendedAway:
            return Utils::Format("%sstatus/xa.png", Web::GetHostUrl<Web::Host::EGL3>());
        case ShowStatus::Offline:
        default:
            return Utils::Format("%sstatus/offline.png", Web::GetHostUrl<Web::Host::EGL3>());
        }
    }
}
