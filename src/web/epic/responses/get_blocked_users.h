#pragma once

namespace EGL3::Web::Epic::Responses {
	struct RespGetBlockedUsers {
		// List of user ids that the user has blocked
		std::vector<std::string> BlockedUsers;

		PARSE_DEFINE(RespGetBlockedUsers)
			PARSE_ITEM_ROOT(BlockedUsers)
		PARSE_END
	};
}
