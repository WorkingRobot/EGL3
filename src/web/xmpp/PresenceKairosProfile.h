#pragma once

#include "../../utils/Crc32.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
	struct PresenceKairosProfile {
		std::string Avatar;

		std::string Background;

		std::optional<std::string> AppInstalled;

		PresenceKairosProfile(const std::string& Avatar, const std::string& Background) :
			Avatar(Avatar),
			Background(Background)
		{}

		PresenceKairosProfile() = default;

		static cpr::Url GetDefaultKairosAvatarUrl() {
			auto Id = rand() % 8 + 1;
			return Web::BaseClient::FormatUrl("https://cdn2.unrealengine.com/Kairos/portraits/cid_%03d_athena_commando_%c_default.png?preview=1", Id, Id > 4 ? 'm' : 'f');
		}

		static cpr::Url GetKairosAvatarUrl(const std::string& Avatar) {
			if (Avatar.empty()) {
				return GetDefaultKairosAvatarUrl();
			}
			return Web::BaseClient::FormatUrl("https://cdn2.unrealengine.com/Kairos/portraits/%s.png?preview=1", Avatar.c_str());
		}

		constexpr static const char* GetDefaultKairosBackgroundUrl() {
			return "https://epic.gl/assets/backgrounds/C893C04B.png";
		}

		static cpr::Url GetKairosBackgroundUrl(const std::string& Background) {
			if (Background.empty()) {
				return GetDefaultKairosBackgroundUrl();
			}
			auto Hash = Utils::Crc32(Background.c_str(), Background.size());
			return Web::BaseClient::FormatUrl("https://epic.gl/assets/backgrounds/%04X.png", Hash);
		}

		rapidjson::StringBuffer ToProperty() const {
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

		PARSE_DEFINE(PresenceKairosProfile)
			PARSE_ITEM_DEF("avatar", Avatar, "")
			PARSE_ITEM_DEF("avatarBackground", Background, "")
			PARSE_ITEM_OPT("appInstalled", AppInstalled)
		PARSE_END
	};
}
