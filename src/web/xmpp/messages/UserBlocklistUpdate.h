#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Xmpp::Messages {
	struct UserBlocklistUpdate {
		DEFINE_JSON_ENUM(StatusEnum, BLOCKED, UNBLOCKED)

		// Time of the event
		TimePoint Timestamp;

		// Account id (you)
		std::string Owner;

		// Account id (target)
		std::string Account;

		// "BLOCKED": Blocked account
		// "UNBLOCKED": Unblocked account
		StatusEnumJson Status;

		PARSE_DEFINE(UserBlocklistUpdate)
			PARSE_ITEM("timestamp", Timestamp)
			PARSE_ITEM("ownerId", Owner)
			PARSE_ITEM("accountId", Account)
			PARSE_ITEM("status", Status)
		PARSE_END
	};
}
