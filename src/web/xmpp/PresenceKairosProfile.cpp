#include "PresenceKairosProfile.h"

#include "../../utils/Crc32.h"
#include "../../utils/Format.h"

#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
    PresenceKairosProfile::PresenceKairosProfile(const std::string& Avatar, const std::string& Background) :
        Avatar(Avatar),
        Background(Background)
    {

    }

    std::string PresenceKairosProfile::GetDefaultKairosAvatarUrl() {
        auto Id = rand() % 8 + 1;
        return Utils::Format("https://cdn2.unrealengine.com/Kairos/portraits/cid_%03d_athena_commando_%c_default.png?preview=1", Id, Id > 4 ? 'm' : 'f');
    }

    std::string PresenceKairosProfile::GetKairosAvatarUrl(const std::string& Avatar) {
        if (Avatar.empty()) {
            return GetDefaultKairosAvatarUrl();
        }
        return Utils::Format("https://cdn2.unrealengine.com/Kairos/portraits/%s.png?preview=1", Avatar.c_str());
    }

    constexpr const char* PresenceKairosProfile::GetDefaultKairosBackgroundUrl() {
        return "https://epic.gl/assets/backgrounds/C893C04B.png";
    }

    std::string PresenceKairosProfile::GetKairosBackgroundUrl(const std::string& Background) {
        if (Background.empty()) {
            return GetDefaultKairosBackgroundUrl();
        }
        auto Hash = Utils::Crc32(Background.c_str(), Background.size());
        return Utils::Format("https://epic.gl/assets/backgrounds/%04X.png", Hash);
    }

    rapidjson::StringBuffer PresenceKairosProfile::ToProperty() const {
        rapidjson::StringBuffer Buffer;
        rapidjson::Writer Writer(Buffer);

        Writer.StartObject();

        Writer.Key("avatar");
        Writer.String(Avatar.c_str());

        Writer.Key("avatarBackground");
        Writer.String(Background.c_str());

        if (AppInstalled.has_value()) {
            Writer.Key("appInstalled");
            Writer.String(AppInstalled->c_str(), AppInstalled->size());
        }

        Writer.EndObject();

        return Buffer;
    }
}
