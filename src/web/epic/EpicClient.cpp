#include "EpicClient.h"

namespace EGL3::Web::Epic {
	BaseClient::Response<Responses::GetPageInfo> EpicClient::GetPageInfo(const std::string& Language)
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

		Responses::GetPageInfo Resp;
		if (!Responses::GetPageInfo::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetBlogPosts> EpicClient::GetBlogPosts(const std::string& Locale, int PostsPerPage, int Offset)
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

		Responses::GetBlogPosts Resp;
		if (!Responses::GetBlogPosts::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetStatuspageSummary> EpicClient::GetStatuspageSummary()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://status.epicgames.com/api/v2/summary.json" }
		);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (Response.status_code != 200) {
			return ERROR_CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ERROR_CODE_NOT_JSON;
		}

		Responses::GetStatuspageSummary Resp;
		if (!Responses::GetStatuspageSummary::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}
}