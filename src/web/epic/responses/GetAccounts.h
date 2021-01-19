#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetAccounts {
        struct Account {
            // Account id
            std::string Id;

            // Display name or username
            std::optional<std::string> DisplayName;

            // Key is the auth type, value is the auth data
            std::unordered_map<std::string, GetAccountExternalAuths::ExternalAuth> ExternalAuths;

            // links is an empty dictionary, only some users have it, presumably for social medias?

            const std::string& GetDisplayName() const {
                if (DisplayName.has_value()) {
                    return DisplayName.value();
                }

                for (auto& Auth : ExternalAuths) {
                    if (Auth.second.ExternalDisplayName.has_value()) {
                        return Auth.second.ExternalDisplayName.value();
                    }
                }

                return Id;
            }

            PARSE_DEFINE(Account)
                PARSE_ITEM("id", Id)
                PARSE_ITEM_OPT("displayName", DisplayName)
                PARSE_ITEM("externalAuths", ExternalAuths)
            PARSE_END
        };

        // A list of all accounts requested, note that some can be missing if an id does not exist, or if you are unauthorized to view them
        std::vector<Account> Accounts;

        PARSE_DEFINE(GetAccounts)
            PARSE_ITEM_ROOT(Accounts)
        PARSE_END
    };
}