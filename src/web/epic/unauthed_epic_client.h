#pragma once

#include "base_client.h"

// Since we don't offer any authentication, this client can be used freely
// It can access some party hub features (e.g. shop), news/comics/tournaments/etc,
// lightswitch, Fortnite manifest data, etc.
class UnauthedEpicClient : public BaseClient {
public:
	UnauthedEpicClient();

	// Get Page Info

	Response<RespGetPageInfo> GetPageInfo(const std::string& Language);
};