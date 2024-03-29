#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetAccountExternalAuths {
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

            // Username/id of the user on the external platform (optional)
            std::optional<std::string> ExternalAuthId;

            // Type of the auth id (optional)
            std::optional<std::string> ExternalAuthIdType;

            // Secondary username/id of the external platform (optional)
            std::optional<std::string> ExternalAuthSecondaryId;

            // Display name of the user on the external platform
            std::optional<std::string> ExternalDisplayName;

            // Avatar (id) of the user on the external platform (optional)
            std::optional<std::string> Avatar;

            // List of all auth ids this connection is attached to
            std::vector<AuthId> AuthIds;

            // When this connection was added to the epic account (only given if this is your account)
            std::optional<TimePoint> DateAdded;

            // When this connection was used to login to the epic account (this doesn't look accurate) (optional)
            std::optional<TimePoint> LastLogin;

            PARSE_DEFINE(ExternalAuth)
                PARSE_ITEM("accountId", AccountId)
                PARSE_ITEM("type", Type)
                PARSE_ITEM_OPT("externalAuthId", ExternalAuthId)
                PARSE_ITEM_OPT("externalAuthIdType", ExternalAuthIdType)
                PARSE_ITEM_OPT("externalAuthSecondaryId", ExternalAuthSecondaryId)
                PARSE_ITEM_OPT("externalDisplayName", ExternalDisplayName)
                PARSE_ITEM_OPT("avatar", Avatar)
                PARSE_ITEM("authIds", AuthIds)
                PARSE_ITEM_OPT("dateAdded", DateAdded)
                PARSE_ITEM_OPT("lastLogin", LastLogin)
            PARSE_END
        };

        // A list of all external connections the account has
        std::vector<ExternalAuth> ExternalAuths;

        PARSE_DEFINE(GetAccountExternalAuths)
            PARSE_ITEM_ROOT(ExternalAuths)
        PARSE_END
    };
}