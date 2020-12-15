#pragma once

namespace EGL3::Web::Xmpp::Messages {
	struct FriendshipRequest {
		DEFINE_JSON_ENUM(ReasonEnum, PENDING, ACCEPTED)

		// Time of the event
		TimePoint Timestamp;

		// Account id (always you, even if other user accepted)
		std::string From;

		// Account id (target)
		std::string To;

		// "PENDING": Sent request
		// "ACCEPTED": Accepted request
		ReasonEnumJson Reason;

		PARSE_DEFINE(FriendshipRequest)
			PARSE_ITEM("timestamp", Timestamp)
			PARSE_ITEM("from", From)
			PARSE_ITEM("to", To)
			PARSE_ITEM("reason", Reason)
		PARSE_END
	};
}
