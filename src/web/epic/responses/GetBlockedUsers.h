#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetBlockedUsers {
        // List of all blocked users returned
        std::vector<GetFriendsSummary::BaseUser> Users;

        PARSE_DEFINE(GetBlockedUsers)
            PARSE_ITEM_ROOT(Users)
        PARSE_END
    };
}
