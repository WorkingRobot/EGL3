#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetSettingsForAccounts {
        struct AccountSetting {
            // Account id this pair is assigned to
            std::string AccountId;

            // Name of the setting
            std::string Key;

            // Value of the setting for the given user
            std::string Value;

            PARSE_DEFINE(AccountSetting)
                PARSE_ITEM("accountId", AccountId)
                PARSE_ITEM("key", Key)
                PARSE_ITEM("value", Value)
            PARSE_END
        };

        // List of settings set for the requested accounts
        std::vector<AccountSetting> Values;

        PARSE_DEFINE(GetSettingsForAccounts)
            PARSE_ITEM_ROOT(Values)
        PARSE_END
    };
}
