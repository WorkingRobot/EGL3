#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Stomp::Messages {
    struct PartyPing {
        TimePoint Sent;
        TimePoint Expires;
        std::string Namespace;
        std::string PingerId;
        std::string PingerDisplayName;
        std::string PingerPlatform;
        std::string PingerPlatformDisplayName;
        std::unordered_map<std::string, std::string> Meta;

        PARSE_DEFINE(PartyPing)
            PARSE_ITEM("sent", Sent)
            PARSE_ITEM("ns", Namespace)
            PARSE_ITEM("pinger_id", PingerId)
            PARSE_ITEM("pinger_dn", PingerDisplayName)
            PARSE_ITEM_OPT("pinger_pl", PingerPlatform)
            PARSE_ITEM_OPT("pinger_pl_dn", PingerPlatformDisplayName)
            PARSE_ITEM("expires", Expires)
            PARSE_ITEM("meta", Meta)
        PARSE_END
    };

    /* Example metas:
    { Initial ping (before invite)
        "urn:epic:conn:type_s": "game",
        "urn:epic:member:dn_s": "Asriel_Dev",
        "urn:epic:conn:platform_s": "WIN",
        "urn:epic:cfg:build-id_s": "1:3:16635603",
        "urn:epic:invite:platformdata_s": ""
    }
    { Repeated ping
        "urn:epic:conn:platform_s": "WIN",
        "urn:epic:invite:platformdata_s": ""
    }
    { Repeated ping (from PSN user)
        "urn:epic:conn:platform_s": "PSN",
        "urn:epic:invite:platformdata_s": ""
    }
    */
}
