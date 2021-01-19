#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetFriendsSuggested {
        // List of all suggested friends
        std::vector<GetFriendsSummary::RealFriend> Suggestions;

        PARSE_DEFINE(GetFriendsSuggested)
            PARSE_ITEM_ROOT(Suggestions)
        PARSE_END
    };
}
