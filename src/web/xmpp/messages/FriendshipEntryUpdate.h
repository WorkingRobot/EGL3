#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Xmpp::Messages {
	struct FriendshipEntryUpdate {
		// Time of the event
		TimePoint Timestamp;

		// Account id
		std::string FriendId;

		PARSE_DEFINE(FriendshipEntryUpdate)
			PARSE_ITEM("timestamp", Timestamp)
			PARSE_ITEM("friendId", FriendId)
		PARSE_END
	};
}
