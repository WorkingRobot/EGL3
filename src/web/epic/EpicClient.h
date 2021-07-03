#pragma once

#include "../BaseClient.h"
#include "../Http.h"
#include "../Response.h"
#include "bps/ChunkData.h"
#include "bps/Manifest.h"
#include "responses/GetCsrfToken.h"
#include "responses/GetDownloadInfo.h"
#include "responses/GetPageInfo.h"
#include "responses/GetBlogPosts.h"
#include "responses/GetStatuspageSummary.h"
#include "responses/OAuthToken.h"

namespace EGL3::Web::Epic {
    // Since we don't offer *any* authentication (no client_credentials either!), this
    // client can be used freely. It can access some party hub features (e.g. shop),
    // news/comics/tournaments, blog posts, etc.
    class EpicClient : public BaseClient {
    public:
        Response<Responses::GetPageInfo> GetPageInfo(const std::string& Language);

        Response<Responses::GetBlogPosts> GetBlogPosts(const std::string& Locale, int PostsPerPage = 0, int Offset = 0);

        Response<Responses::GetStatuspageSummary> GetStatuspageSummary();

        Response<BPS::Manifest> GetManifest(const Responses::GetDownloadInfo::Element& Element, std::string& CloudDir);

        Response<BPS::Manifest> GetManifestCacheable(const Responses::GetDownloadInfo::Element& Element, std::string& CloudDir, const std::filesystem::path& CacheDir);

        Response<BPS::ChunkData> GetChunk(const std::string& CloudDir, BPS::FeatureLevel FeatureLevel, const BPS::ChunkInfo& ChunkInfo);

        Response<BPS::ChunkData> GetChunkCacheable(const std::string& CloudDir, BPS::FeatureLevel FeatureLevel, const BPS::ChunkInfo& ChunkInfo, const std::filesystem::path& CacheDir);

        Response<Responses::OAuthToken> AuthorizationCode(const cpr::Authentication& AuthClient, const std::string& Code);

        Response<Responses::OAuthToken> ClientCredentials(const cpr::Authentication& AuthClient);

        Response<Responses::OAuthToken> DeviceAuth(const cpr::Authentication& AuthClient, const std::string& AccountId, const std::string& DeviceId, const std::string& Secret);

        Response<Responses::OAuthToken> ExchangeCode(const cpr::Authentication& AuthClient, const std::string& Code);

        Response<Responses::OAuthToken> ExchangeCodeEOS(const cpr::Authentication& AuthClient, const std::string& DeploymentId, const std::string& Code);

        Response<Responses::OAuthToken> RefreshToken(const cpr::Authentication& AuthClient, const std::string& Token);

        Response<Responses::GetCsrfToken> GetCsrfToken();

    private:
        template<typename ResponseType, int SuccessStatusCode, class CallFunctorType>
        Response<ResponseType> Call(CallFunctorType&& CallFunctor);
    };
}