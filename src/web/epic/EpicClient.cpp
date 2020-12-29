#include "EpicClient.h"

#include "../Http.h"
#include "../RunningFunctionGuard.h"

namespace EGL3::Web::Epic {
	Response<Responses::GetPageInfo> EpicClient::GetPageInfo(const std::string& Language)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl<Host::FortniteContent>("pages/fortnite-game"),
			cpr::Parameters{ { "lang", Language } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetPageInfo Resp;
		if (!Responses::GetPageInfo::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetBlogPosts> EpicClient::GetBlogPosts(const std::string& Locale, int PostsPerPage, int Offset)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl<Host::BaseFortnite>("blog/getPosts"),
			cpr::Parameters{ { "category", "" }, { "postsPerPage", std::to_string(PostsPerPage) }, { "offset", std::to_string(Offset) }, { "locale", Locale }, { "rootPageSlug", "blog" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetBlogPosts Resp;
		if (!Responses::GetBlogPosts::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetStatuspageSummary> EpicClient::GetStatuspageSummary()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl<Host::Statuspage>("v2/summary.json")
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetStatuspageSummary Resp;
		if (!Responses::GetStatuspageSummary::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}
}