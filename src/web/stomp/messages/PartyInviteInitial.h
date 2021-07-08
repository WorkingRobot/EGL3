#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Stomp::Messages {
    struct PartyInviteInitial {
        TimePoint Sent;
        std::string Namespace;
        std::string PartyId;
        std::string InviterId;
        std::string InviterDisplayName;
        std::string InviteeId;
        TimePoint SentAt;
        TimePoint UpdatedAt;
        std::vector<std::string> FriendsIds;
        int MembersCount;
        std::unordered_map<std::string, std::string> Meta;

        PARSE_DEFINE(PartyInviteInitial)
            PARSE_ITEM("sent", Sent)
            PARSE_ITEM("ns", Namespace)
            PARSE_ITEM("party_id", PartyId)
            PARSE_ITEM("inviter_id", InviterId)
            PARSE_ITEM("inviter_dn", InviterDisplayName)
            PARSE_ITEM("invitee_id", InviteeId)
            PARSE_ITEM("sent_at", SentAt)
            PARSE_ITEM("updated_at", UpdatedAt)
            PARSE_ITEM("friends_ids", FriendsIds)
            PARSE_ITEM("members_count", MembersCount)
            PARSE_ITEM("meta", Meta)
        PARSE_END
    };

    /* Example metas:
    {
        "urn:epic:conn:type_s": "game",
        "urn:epic:member:dn_s": "Asriel_Dev",
        "urn:epic:conn:platform_s": "WIN",
        "urn:epic:cfg:build-id_s": "1:3:16635603",
        "urn:epic:invite:platformdata_s": ""
    }
    */
}
