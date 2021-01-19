#pragma once

#include "../BaseClient.h"
#include "../Response.h"
#include "bps/Manifest.h"
#include "responses/GetDownloadInfo.h"
#include "responses/GetPageInfo.h"
#include "responses/GetBlogPosts.h"
#include "responses/GetStatuspageSummary.h"

namespace EGL3::Web::Epic {
    // Since we don't offer *any* authentication (no client_credentials either!), this
    // client can be used freely. It can access some party hub features (e.g. shop),
    // news/comics/tournaments, blog posts, etc.
    class EpicClient : public BaseClient {
    public:
        Response<Responses::GetPageInfo> GetPageInfo(const std::string& Language);

        Response<Responses::GetBlogPosts> GetBlogPosts(const std::string& Locale, int PostsPerPage = 0, int Offset = 0);

        Response<Responses::GetStatuspageSummary> GetStatuspageSummary();

        Response<BPS::Manifest> GetManifest(const Responses::GetDownloadInfo::Manifest& Manifest);

    private:
        template<typename ResponseType, int SuccessStatusCode, class CallFunctorType>
        Response<ResponseType> Call(CallFunctorType&& CallFunctor);
    };
}