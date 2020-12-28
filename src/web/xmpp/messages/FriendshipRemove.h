#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Xmpp::Messages {
	struct FriendshipRemove {
		DEFINE_JSON_ENUM(ReasonEnum, ABORTED, REJECTED, DELETED)

		// Time of the event
		TimePoint Timestamp;

		// Account id (you)
		std::string From;

		// Account id (target)
		std::string To;

		// "ABORTED": Cancel outbound request
		// "REJECTED": Decline inbound request
		// "DELETED": Remove existing friend
		ReasonEnumJson Reason;

		PARSE_DEFINE(FriendshipRemove)
			PARSE_ITEM("timestamp", Timestamp)
			PARSE_ITEM("from", From)
			PARSE_ITEM("to", To)
			PARSE_ITEM("reason", Reason)
		PARSE_END
	};
}
