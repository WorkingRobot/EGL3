#pragma once

struct RespGetFriends {
	struct Connection {
		// Account id of the user on the third party platform
		std::string Id;

		// Name of the user on the third party platform
		std::string Name;

		// URL of their avatar on the third party platform
		std::string Avatar;

		// Where the connection came from (e.g. steam)
		std::string Source;

		PARSE_DEFINE(Connection)
			PARSE_ITEM("id", Id)
			PARSE_ITEM("name", Name)
			PARSE_ITEM("avatar", Avatar)
			PARSE_ITEM("source", Source)
		PARSE_END
	};

	struct Friend {
		// Account id of the friend
		std::string AccountId;

		// Status of the relation betweeen the user and the friend (e.g. "ACCEPTED" "PENDING" "SUGGESTED")
		std::string Status;

		// Direction of who started the relation between the user and the friend
		// (e.g. "INBOUND" = they sent request first and "OUTBOUND" = vice versa)
		std::string Direction;

		// Alias that the friend goes by (set by the user)
		std::optional<std::string> Alias;

		// When the relation was created
		TimePoint Created;

		// Whether the user marked them as favorite
		std::optional<bool> Favorite;

		// Connections that the user and the friend has (only appears when "SUGGESTED")
		// e.g. steam accounts
		std::optional<std::vector<Connection>> Connections;

		PARSE_DEFINE(Friend)
			PARSE_ITEM("accountId", AccountId)
			PARSE_ITEM("status", Status)
			PARSE_ITEM("direction", Direction)
			PARSE_ITEM_OPT("alias", Alias)
			PARSE_ITEM("created", Created)
			PARSE_ITEM_OPT("favorite", Favorite)
			PARSE_ITEM_OPT("connections", Connections)
		PARSE_END
	};

	// List of all friends returned
	std::vector<Friend> Friends;

	PARSE_DEFINE(RespGetFriends)
		PARSE_ITEM_ROOT(Friends)
	PARSE_END
};