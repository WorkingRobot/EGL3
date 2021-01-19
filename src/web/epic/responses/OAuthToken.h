#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct OAuthToken {
        // Access token
        std::string AccessToken;

        // Seconds until access token expires
        int ExpiresIn;

        // Time when access token expired
        TimePoint ExpiresAt;

        // Type of token returned, I only observe "bearer"
        std::string TokenType;

        // Refresh token
        std::optional<std::string> RefreshToken;

        // Seconds until refresh token expires
        std::optional<int> RefreshExpiresIn;

        // Time when refresh token expired
        std::optional<TimePoint> RefreshExpiresAt;

        // Account id
        std::optional<std::string> AccountId;

        // Client id used to authenticate
        std::string ClientId;

        // Whether this is an internal client (i think this is set to false when using something external like TFN's token or FN community battles)
        bool IsInternalClient;

        // Client service (i've seen values like "fortnite" "valkyrie" "launcher")
        std::string ClientService;

        // Display name of user
        std::optional<std::string> DisplayName;

        // App of the client, not too sure what this exactly is
        std::optional<std::string> App;

        // Id of the user inside the app, i've only seen this be set to the account id, but it could be different?
        std::optional<std::string> InAppId;


        PARSE_DEFINE(OAuthToken)
            PARSE_ITEM("access_token", AccessToken)
            PARSE_ITEM("expires_in", ExpiresIn)
            PARSE_ITEM("expires_at", ExpiresAt)
            PARSE_ITEM("token_type", TokenType)
            PARSE_ITEM_OPT("refresh_token", RefreshToken)
            PARSE_ITEM_OPT("refresh_expires", RefreshExpiresIn)
            PARSE_ITEM_OPT("refresh_expires_at", RefreshExpiresAt)
            PARSE_ITEM_OPT("account_id", AccountId)
            PARSE_ITEM("client_id", ClientId)
            PARSE_ITEM("internal_client", IsInternalClient)
            PARSE_ITEM("client_service", ClientService)
            PARSE_ITEM_OPT("displayName", DisplayName)
            PARSE_ITEM_OPT("app", App)
            PARSE_ITEM_OPT("in_app_id", InAppId)
        PARSE_END
    };
}
