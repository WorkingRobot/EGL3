#include "EpicClient.h"

#include "../Http.h"
#include "../RunningFunctionGuard.h"

namespace EGL3::Web::Epic {
    Response<Responses::GetPageInfo> EpicClient::GetPageInfo(const std::string& Language)
    {
        return Call<Responses::GetPageInfo, 200>(
            [&]() {
                return Http::Get(
                    Http::FormatUrl<Host::FortniteContent>("pages/fortnite-game"),
                    cpr::Parameters{ { "lang", Language } }
                );
            }
        );
    }

    Response<Responses::GetBlogPosts> EpicClient::GetBlogPosts(const std::string& Locale, int PostsPerPage, int Offset)
    {
        return Call<Responses::GetBlogPosts, 200>(
            [&]() {
                return Http::Get(
                    Http::FormatUrl<Host::BaseFortnite>("blog/getPosts"),
                    cpr::Parameters{ { "category", "" }, { "postsPerPage", std::to_string(PostsPerPage) }, { "offset", std::to_string(Offset) }, { "locale", Locale }, { "rootPageSlug", "blog" } }
                );
            }
        );
    }

    Response<Responses::GetStatuspageSummary> EpicClient::GetStatuspageSummary()
    {
        return Call<Responses::GetStatuspageSummary, 200>(
            []() {
                return Http::Get(
                    Http::FormatUrl<Host::Statuspage>("v2/summary.json")
                );
            }
        );
    }

    Response<BPS::Manifest> EpicClient::GetManifest(const Responses::GetDownloadInfo::Manifest& Manifest)
    {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        // Some really special people decided to think that using url encoded parameters should return a 403
        // Now, I'm forced to use a ostringstream and manually append parameters. Now look what you've done!
        std::ostringstream BuiltUri;
        BuiltUri << Manifest.Uri << "?";
        for (auto& Param : Manifest.QueryParams) {
            BuiltUri << Param.Name << "=" << Param.Value << "&";
        }

        cpr::Header Headers;
        for (auto& Header : Manifest.Headers) {
            Headers.emplace(Header.Name, Header.Value);
        }

        auto Response = Http::Get(
            cpr::Url{ BuiltUri.str().c_str(),  (size_t)BuiltUri.tellp() - 1 },
            Headers
        );

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != 200) {
            return Response.status_code;
        }

        return BPS::Manifest(Response.text.data(), Response.text.size());
    }

    template<typename ResponseType, int SuccessStatusCode, class CallFunctorType>
    Response<ResponseType> EpicClient::Call(CallFunctorType&& CallFunctor) {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        auto Response = std::invoke(std::move(CallFunctor));

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != SuccessStatusCode) {
            return Response.status_code;
        }

        if constexpr (std::is_void_v<ResponseType>) {
            return ErrorData::Status::Success;
        }
        else {
            auto RespJson = Http::ParseJson(Response);

            if (RespJson.HasParseError()) {
                return ErrorData::Status::NotJson;
            }

            ResponseType Resp;
            if (!ResponseType::Parse(RespJson, Resp)) {
                return ErrorData::Status::BadJson;
            }

            return Resp;
        }
    }
}