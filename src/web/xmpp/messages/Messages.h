#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Xmpp::Messages {
	using namespace EGL3::Web::Json;
}

#include "FriendshipRemove.h"
#include "FriendshipRequest.h"
#include "UserBlocklistUpdate.h"

#undef PARSE_DEFINE
#undef PARSE_END
#undef PARSE_ITEM
#undef PARSE_ITEM_OPT
#undef PARSE_ITEM_ROOT
