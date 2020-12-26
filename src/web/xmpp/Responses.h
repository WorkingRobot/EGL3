#pragma once

#include "../JsonParsing.h"

namespace EGL3::Web::Xmpp::Json {
	using namespace EGL3::Web::Json;
}

#include "Presence.h"
#include "PresenceKairosProfile.h"
#include "PresenceProperties.h"
#include "PresenceStatus.h"
#include "ResourceId.h"
#include "ShowStatus.h"

#undef PARSE_DEFINE
#undef PARSE_END
#undef PARSE_ITEM
#undef PARSE_ITEM_OPT
#undef PARSE_ITEM_ROOT
