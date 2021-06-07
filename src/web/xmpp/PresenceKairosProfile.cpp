#include "PresenceKairosProfile.h"

#include "../../utils/Crc32.h"
#include "../../utils/Random.h"
#include "../../web/Hosts.h"

#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
    PresenceKairosProfile::PresenceKairosProfile(const std::string& Avatar, const std::string& Background) :
        Avatar(Avatar),
        Background(Background)
    {

    }

    std::string PresenceKairosProfile::GetDefaultKairosAvatar() {
        uint32_t Id = Utils::Random(1, 8);
        return std::format("cid_{:03}_athena_commando_{}_default", Id, Id > 4 ? 'm' : 'f');
    }

    std::string PresenceKairosProfile::GetKairosAvatarUrl(const std::string& Avatar) {
        if (Avatar.empty()) {
            return GetKairosAvatarUrl(GetDefaultKairosAvatar());
        }
        return std::format("{}Kairos/portraits/{}.png?preview=1", Web::GetHostUrl<Web::Host::UnrealEngineCdn2>(), Avatar);
    }

    std::string PresenceKairosProfile::GetDefaultKairosBackground() {
        return "[\"#AEC1D3\",\"#687B8E\",\"#36404A\"]";
    }

    std::string PresenceKairosProfile::GetKairosBackgroundUrl(const std::string& Background) {
        if (Background.empty()) {
            return GetKairosBackgroundUrl(GetDefaultKairosBackground());
        }
        auto Hash = Utils::Crc32(Background.c_str(), Background.size());
        return std::format("{}backgrounds/{:04X}.png", Web::GetHostUrl<Web::Host::EGL3>(), Hash);
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
