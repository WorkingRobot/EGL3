#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetFriends {
        // List of all friends returned
        std::vector<GetFriendsSummary::RealFriend> Friends;

        PARSE_DEFINE(GetFriends)
            PARSE_ITEM_ROOT(Friends)
        PARSE_END
    };
}
