#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetFriendsRequested {
		// List of all requested friends returned (for incoming and outgoing)
		std::vector<GetFriendsSummary::RealFriend> Requests;

		PARSE_DEFINE(GetFriendsRequested)
			PARSE_ITEM_ROOT(Requests)
		PARSE_END
	};
}
