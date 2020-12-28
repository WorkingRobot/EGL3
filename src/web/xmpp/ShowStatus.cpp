#include "ShowStatus.h"

namespace EGL3::Web::Xmpp::Json {
	constexpr const char* ShowStatusToString(ShowStatus Status) {
		switch (Status)
		{
		case ShowStatus::DoNotDisturb:
			return "Do Not Disturb";
		case ShowStatus::Chat:
			return "Chat";
		case ShowStatus::Online:
			return "Online";
		case ShowStatus::Away:
			return "Away";
		case ShowStatus::ExtendedAway:
			return "Extended Away";
		case ShowStatus::Offline:
			return "Offline";
		default:
			return "Unknown";
		}
	}

	constexpr const char* ShowStatusToUrl(ShowStatus Status) {
		switch (Status)
		{
		case ShowStatus::Chat:
			return "https://epic.gl/assets/status/chat.png";
		case ShowStatus::DoNotDisturb:
			return "https://epic.gl/assets/status/dnd.png";
		case ShowStatus::Online:
			return "https://epic.gl/assets/status/online.png";
		case ShowStatus::Away:
			return "https://epic.gl/assets/status/away.png";
		case ShowStatus::ExtendedAway:
			return "https://epic.gl/assets/status/xa.png";
		case ShowStatus::Offline:
		default:
			return "https://epic.gl/assets/status/offline.png";
		}
	}
}
