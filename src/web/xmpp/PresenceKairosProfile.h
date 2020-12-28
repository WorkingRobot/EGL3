#pragma once

#include "../BaseClient.h"
#include "../JsonParsing.h"

#include <rapidjson/stringbuffer.h>

namespace EGL3::Web::Xmpp::Json {
    struct PresenceKairosProfile {
        std::string Avatar;

        std::string Background;

        std::optional<std::string> AppInstalled;

        PresenceKairosProfile() = default;

        PresenceKairosProfile(const std::string& Avatar, const std::string& Background);

        static std::string GetDefaultKairosAvatarUrl();

        static std::string GetKairosAvatarUrl(const std::string& Avatar);

        constexpr static const char* GetDefaultKairosBackgroundUrl();

        static std::string GetKairosBackgroundUrl(const std::string& Background);

        rapidjson::StringBuffer ToProperty() const;

        PARSE_DEFINE(PresenceKairosProfile)
            PARSE_ITEM_DEF("avatar", Avatar, "")
            PARSE_ITEM_DEF("avatarBackground", Background, "")
            PARSE_ITEM_OPT("appInstalled", AppInstalled)
        PARSE_END
    };
}
