#pragma once

struct RespGetAccountExternalAuths {
	struct AuthId {
		// Account id of the auth
		std::string Id;

		// Type of auth
		std::string Type;

		PARSE_DEFINE(AuthId)
			PARSE_ITEM("id", Id)
			PARSE_ITEM("type", Type)
		PARSE_END
	};

	struct ExternalAuth {
		// Id of the epic account this auth is attatched to (redundant info since you need it to get this data anyway)
		std::string AccountId;

		// Type of connection (github, google, steam, twitch, etc)
		std::string Type;

		// Username/id of the user on the external platform
		std::string ExternalAuthId;

		// Type of the auth id (optional)
		std::optional<std::string> ExternalAuthIdType;

		// Secondary username/id of the external platform (optional)
		std::optional<std::string> ExternalAuthSecondaryId;

		// Display name of the user on the external platform
		std::string ExternalDisplayName;

		// Avatar (id) of the user on the external platform (optional)
		std::optional<std::string> Avatar;

		// List of all auth ids this connection is attatched to
		std::vector<AuthId> AuthIds;

		// When this connection was added to the epic account
		TimePoint DateAdded;

		// When this connection was used to login to the epic account (this doesn't look accurate) (optional)
		std::optional<TimePoint> LastLogin;

		PARSE_DEFINE(ExternalAuth)
			PARSE_ITEM("accountId", AccountId)
			PARSE_ITEM("type", Type)
			PARSE_ITEM("externalAuthId", ExternalAuthId)
			PARSE_ITEM_OPT("externalAuthIdType", ExternalAuthIdType)
			PARSE_ITEM_OPT("externalAuthSecondaryId", ExternalAuthSecondaryId)
			PARSE_ITEM("externalDisplayName", ExternalDisplayName)
			PARSE_ITEM_OPT("avatar", Avatar)
			PARSE_ITEM("authIds", AuthIds)
			PARSE_ITEM("dateAdded", DateAdded)
			PARSE_ITEM_OPT("lastLogin", LastLogin)
		PARSE_END
	};

	// A list of all external connections the account has
	std::vector<ExternalAuth> ExternalAuths;

	PARSE_DEFINE(RespGetAccountExternalAuths)
		PARSE_ITEM_ROOT(ExternalAuths)
	PARSE_END
};