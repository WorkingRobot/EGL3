#pragma once

#include <stdint.h>

namespace EGL3::Web::Xmpp::Json {
	// There's a bunch of enums that this is based off of,
	// but the most high level one is EOnlinePresenceState
	// https://github.com/EpicGames/UnrealEngine/blob/f8f4b403eb682ffc055613c7caf9d2ba5df7f319/Engine/Source/Runtime/Online/XMPP/Private/XmppJingle/XmppPresenceJingle.cpp#L170
	enum class ShowStatus : uint8_t {
		Offline,
		ExtendedAway, // xa
		Away, // away
		Online,
		DoNotDisturb, // dnd
		Chat, // chat
	};

	static constexpr const char* ShowStatusToString(ShowStatus Status) {
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

	static constexpr const char* ShowStatusToUrl(ShowStatus Status) {
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
