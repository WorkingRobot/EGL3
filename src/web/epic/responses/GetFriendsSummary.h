#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetFriendsSummary {
		struct Connection {
			// Given by all but nintendo
			std::optional<std::string> Name;

			// Given by steam
			std::optional<std::string> Id;

			// Given by steam
			std::optional<std::string> Avatar;
			
			PARSE_DEFINE(Connection)
				PARSE_ITEM_OPT("name", Name)
				PARSE_ITEM_OPT("id", Id)
				PARSE_ITEM_OPT("avatar", Avatar)
			PARSE_END
		};

		struct BaseUser {
			// Account id of the friend
			std::string AccountId;

			// Display name of the user (if not headless)
			std::optional<std::string> DisplayName;

			PARSE_DEFINE(BaseUser)
				PARSE_ITEM("accountId", AccountId)
				PARSE_ITEM_OPT("displayName", DisplayName)
			PARSE_END
		};

		struct SuggestedFriend : public BaseUser {
			// Any external connections the friend has
			std::unordered_map<std::string, Connection> Connections;

			PARSE_DEFINE(SuggestedFriend)
				PARSE_BASE(BaseUser)
				PARSE_ITEM_OPT("connections", Connections)
			PARSE_END
		};

		struct RequestedFriend : public SuggestedFriend {
			// favorite is always false

			PARSE_DEFINE(RequestedFriend)
				PARSE_BASE(SuggestedFriend)
			PARSE_END
		};

		// Returned in the explicit (non-summary) endpoints
		struct BaseFriend : public RequestedFriend {
			// empty groups array

			std::string Alias;

			std::string Note;

			TimePoint Created;

			PARSE_DEFINE(BaseFriend)
				PARSE_BASE(RequestedFriend)
				PARSE_ITEM("alias", Note)
				PARSE_ITEM("note", Alias)
				PARSE_ITEM("created", Created)
			PARSE_END
		};

		struct RealFriend : public BaseFriend {
			int MutualFriendCount;

			PARSE_DEFINE(RealFriend)
				PARSE_BASE(BaseFriend)
				PARSE_ITEM("mutual", MutualFriendCount)
			PARSE_END
		};

		struct Limits {
			bool Incoming;

			bool Outgoing;

			bool Accepted;

			PARSE_DEFINE(Limits)
				PARSE_ITEM("incoming", Incoming)
				PARSE_ITEM("outgoing", Outgoing)
				PARSE_ITEM("accepted", Accepted)
			PARSE_END
		};

		// List of all friends returned
		std::vector<RealFriend> Friends;

		std::vector<RequestedFriend> Incoming;

		std::vector<RequestedFriend> Outgoing;

		std::vector<SuggestedFriend> Suggested;

		std::vector<BaseUser> Blocklist;

		// settings aren't used anywhere, it seems like "acceptInvites" is always "public"

		Limits LimitsReached;

		PARSE_DEFINE(GetFriendsSummary)
			PARSE_ITEM("friends", Friends)
			PARSE_ITEM("incoming", Incoming)
			PARSE_ITEM("outgoing", Outgoing)
			PARSE_ITEM("suggested", Suggested)
			PARSE_ITEM("blocklist", Blocklist)
			PARSE_ITEM("limitsReached", LimitsReached)
		PARSE_END
	};
}
