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

	constexpr const char* ShowStatusToString(ShowStatus Status);

	constexpr const char* ShowStatusToUrl(ShowStatus Status);
}
