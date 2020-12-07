#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
	using namespace EGL3::Web::Json;
}

// Authorized client

#include "GetAccount.h"
#include "GetAccountExternalAuths.h"
#include "GetAccounts.h"
#include "GetAssets.h"
#include "GetAvailableSettingValues.h"
#include "GetBlockedUsers.h"
#include "GetCatalogItems.h"
#include "GetCurrencies.h"
#include "GetDefaultBillingAccount.h"
#include "GetDeviceAuths.h"
#include "GetDownloadInfo.h"
#include "GetEntitlements.h"
#include "GetExternalSourceSettings.h"
#include "GetFriends.h"
#include "GetLightswitchStatus.h"
#include "GetSettingsForAccounts.h"
#include "OAuthToken.h"

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
