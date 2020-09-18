#pragma once

#include "../BaseClient.h"
#include "responses/Responses.h"

namespace EGL3::Web::Epic {
	// Since we don't offer *any* authentication (no client_credentials either!), this
	// client can be used freely. It can access some party hub features (e.g. shop),
	// news/comics/tournaments, blog posts, etc.
	class EpicClient : public BaseClient {
	public:
		// Get Page Info

		Response<Responses::GetPageInfo> GetPageInfo(const std::string& Language);

		// Blog Posts

		Response<Responses::GetBlogPosts> GetBlogPosts(const std::string& Locale, int PostsPerPage = 0, int Offset = 0);
	};
}