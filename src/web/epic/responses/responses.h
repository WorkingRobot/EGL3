#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
	using namespace EGL3::Web::Json;
}

// Authorized client

#include "OAuthToken.h"
#include "GetAccount.h"
#include "GetAccountExternalAuths.h"
#include "GetAccounts.h"
#include "GetDeviceAuths.h"

#include "GetDefaultBillingAccount.h"
#include "GetAssets.h"
#include "GetDownloadInfo.h"

#include "GetCurrencies.h"
#include "GetCatalogItems.h"

#include "GetEntitlements.h"

#include "GetFriendsSummary.h"
#include "GetFriendsRequested.h"
#include "GetFriendsSuggested.h"
#include "GetFriends.h"
#include "GetBlockedUsers.h"

#include "GetAvailableSettingValues.h"
#include "GetSettingsForAccounts.h"

#include "GetLightswitchStatus.h"

// Authorized client (MCP)

#include "QueryProfile.h"

// Unauthorized client

#include "GetBlogPosts.h"
#include "GetPageInfo.h"
#include "GetStatuspageSummary.h"

#undef PARSE_DEFINE
#undef PARSE_END
#undef PARSE_ITEM
#undef PARSE_ITEM_OPT
#undef PARSE_ITEM_ROOT
