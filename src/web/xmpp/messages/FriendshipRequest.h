#pragma once

namespace EGL3::Web::Xmpp::Messages {
	struct FriendshipRequest {
		DEFINE_JSON_ENUM(ReasonEnum, PENDING, ACCEPTED)

		// Time of the event
		TimePoint Timestamp;

		// Account id (the user who initiated the request)
		std::string From;

		// Account id (the other user who got the request)
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
