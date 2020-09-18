#pragma once

#include "base_client.h"
#include "responses/responses.h"

// Since we don't offer *any* authentication (no client_credentials either!), this
// client can be used freely. It can access some party hub features (e.g. shop),
// news/comics/tournaments, blog posts, etc.
class UnauthedEpicClient : public BaseClient {
public:
	// Get Page Info

	Response<RespGetPageInfo> GetPageInfo(const std::string& Language);

	// Blog Posts

	Response<RespGetBlogPosts> GetBlogPosts(const std::string& Locale, int PostsPerPage = 0, int Offset = 0);
};