#pragma once

#include "../../web/JsonParsing.h"

namespace EGL3::Utils::EGL {
    struct RememberMeData {
        struct Profile {
            std::string Region;
            std::string Email;
            std::string FirstName;
            std::string LastName;
            std::string DisplayName;
            std::string Token;
            bool HasPasswordAuth;

            PARSE_DEFINE(Profile)
                PARSE_ITEM("Region", Region)
                PARSE_ITEM("Email", Email)
                PARSE_ITEM("Name", FirstName)
                PARSE_ITEM("LastName", LastName)
                PARSE_ITEM("DisplayName", DisplayName)
                PARSE_ITEM("Token", Token)
                PARSE_ITEM("bHasPasswordAuth", HasPasswordAuth)
            PARSE_END
        };

        std::vector<Profile> Profiles;

        PARSE_DEFINE(RememberMeData)
            PARSE_ITEM_ROOT(Profiles)
        PARSE_END
    };

    class RememberMe {
    public:
        RememberMe();

        RememberMeData::Profile* GetProfile();

        // void ReplaceToken(const std::string& NewToken);

    private:
        static std::filesystem::path GetIniPath();

        std::optional<RememberMeData::Profile> Profile;
    };
}
