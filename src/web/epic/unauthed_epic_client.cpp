#include "unauthed_epic_client.h"

BaseClient::Response<RespGetPageInfo> UnauthedEpicClient::GetPageInfo(const std::string& Language)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://fortnitecontent-website-prod07.ol.epicgames.com/content/api/pages/fortnite-game" },
		cpr::Parameters{ { "lang", Language } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetPageInfo Resp;
	if (!RespGetPageInfo::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

BaseClient::Response<RespGetBlogPosts> UnauthedEpicClient::GetBlogPosts(const std::string& Locale, int PostsPerPage, int Offset)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://www.epicgames.com/fortnite/api/blog/getPosts" },
		cpr::Parameters{ { "category", "" }, { "postsPerPage", std::to_string(PostsPerPage) }, { "offset", std::to_string(Offset) }, { "locale", Locale }, { "rootPageSlug", "blog" } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetBlogPosts Resp;
	if (!RespGetBlogPosts::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}