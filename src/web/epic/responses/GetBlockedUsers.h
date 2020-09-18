#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetBlockedUsers {
		// List of user ids that the user has blocked
		std::vector<std::string> BlockedUsers;

		PARSE_DEFINE(GetBlockedUsers)
			PARSE_ITEM_ROOT(BlockedUsers)
		PARSE_END
	};
}
