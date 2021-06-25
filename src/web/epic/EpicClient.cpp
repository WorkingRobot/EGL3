#include "EpicClient.h"

#include "../../utils/streams/FileStream.h"
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

    cpr::Response GetManifestData(const Responses::GetDownloadInfo::Manifest& Manifest)
    {
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

        return Http::Get(
            cpr::Url{ BuiltUri.str().c_str(),  (size_t)BuiltUri.tellp() - 1 },
            Headers
        );
    }

    Response<BPS::Manifest> EpicClient::GetManifest(const Responses::GetDownloadInfo::Element& Element, std::string& CloudDir)
    {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        auto& Manifest = Element.PickManifest();
        CloudDir = Manifest.GetCloudDir();

        auto Response = GetManifestData(Manifest);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != 200) {
            return Response.status_code;
        }

        return BPS::Manifest(Response.text.data(), Response.text.size());
    }

    Response<BPS::Manifest> EpicClient::GetManifestCacheable(const Responses::GetDownloadInfo::Element& Element, std::string& CloudDir, const std::filesystem::path& CacheDir)
    {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        auto& Manifest = Element.PickManifest();
        CloudDir = Manifest.GetCloudDir();

        auto CachedPath = (CacheDir / Element.Hash).replace_extension("manifest");
        std::error_code Error;
        if (std::filesystem::is_regular_file(CachedPath, Error)) {
            Utils::Streams::FileStream Stream;
            if (Stream.open(CachedPath, "rb")) {
                // Using an intermediate memory buffer speeds up the manifest parsing significantly
                // RapidJSON loves calling .Tell() which singlehandedly slowed down parsing by over 1.5s
                auto Buf = std::make_unique<char[]>(Stream.size());
                Stream.read(Buf.get(), Stream.size());
                BPS::Manifest Ret(Buf.get(), Stream.size());
                if (!Ret.HasError()) {
                    return std::move(Ret);
                }
            }
        }

        auto Response = GetManifestData(Manifest);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != 200) {
            return Response.status_code;
        }

        {
            Utils::Streams::FileStream Stream;
            if (Stream.open(CachedPath, "wb")) {
                Stream.write(Response.text.data(), Response.text.size());
            }
        }

        return BPS::Manifest(Response.text.data(), Response.text.size());
    }

    cpr::Response GetChunkData(const std::string& CloudDir, BPS::FeatureLevel FeatureLevel, const BPS::ChunkInfo& ChunkInfo)
    {
        return Http::Get(
            cpr::Url{ std::format("{}/{}/{:02}/{:016X}_{:08X}{:08X}{:08X}{:08X}.chunk",
                CloudDir.c_str(),
                BPS::GetChunkSubdir(FeatureLevel),
                ChunkInfo.GroupNumber,
                ChunkInfo.Hash,
                ChunkInfo.Guid.A, ChunkInfo.Guid.B, ChunkInfo.Guid.C, ChunkInfo.Guid.D
            ) }
        );
    }

    Response<BPS::ChunkData> EpicClient::GetChunk(const std::string& CloudDir, BPS::FeatureLevel FeatureLevel, const BPS::ChunkInfo& ChunkInfo)
    {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (FeatureLevel < BPS::FeatureLevel::DataFileRenames) {
            // return too old, don't care enough to support this
            // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/BuildPatchServices/Private/BuildPatchUtil.cpp#L70
            return 400;
        }

        auto Response = GetChunkData(CloudDir, FeatureLevel, ChunkInfo);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != 200) {
            return Response.status_code;
        }

        return BPS::ChunkData(Response.text.data(), Response.text.size());
    }

    Response<BPS::ChunkData> EpicClient::GetChunkCacheable(const std::string& CloudDir, BPS::FeatureLevel FeatureLevel, const BPS::ChunkInfo& ChunkInfo, const std::filesystem::path& CacheDir)
    {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (FeatureLevel < BPS::FeatureLevel::DataFileRenames) {
            // return too old, don't care enough to support this
            // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/BuildPatchServices/Private/BuildPatchUtil.cpp#L70
            return 400;
        }

        auto CachedPath = (CacheDir / std::format("{:08X}{:08X}{:08X}{:08X}", ChunkInfo.Guid.A, ChunkInfo.Guid.B, ChunkInfo.Guid.C, ChunkInfo.Guid.D)).replace_extension("chunk");
        std::error_code Error;
        if (std::filesystem::is_regular_file(CachedPath, Error)) {
            Utils::Streams::FileStream Stream;
            if (Stream.open(CachedPath, "rb")) {
                BPS::ChunkData Ret(Stream);
                if (!Ret.HasError()) {
                    return std::move(Ret);
                }
            }
        }

        auto Response = GetChunkData(CloudDir, FeatureLevel, ChunkInfo);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != 200) {
            return Response.status_code;
        }

        {
            Utils::Streams::FileStream Stream;
            if (Stream.open(CachedPath, "wb")) {
                Stream.write(Response.text.data(), Response.text.size());
            }
        }

        return BPS::ChunkData(Response.text.data(), Response.text.size());
    }

    Response<Responses::OAuthToken> EpicClient::AuthorizationCode(const cpr::Authentication& AuthClient, const std::string& Code)
    {
        return Call<Responses::OAuthToken, 200>(
            [&]() {
                return Http::Post(
                    Http::FormatUrl<Host::Account>("oauth/token"),
                    AuthClient,
                    cpr::Payload{ { "grant_type", "authorization_code" }, { "token_type", "eg1" }, { "code", Code } }
                );
            }
        );
    }

    Response<Responses::OAuthToken> EpicClient::ClientCredentials(const cpr::Authentication& AuthClient)
    {
        return Call<Responses::OAuthToken, 200>(
            [&]() {
                return Http::Post(
                    Http::FormatUrl<Host::Account>("oauth/token"),
                    AuthClient,
                    cpr::Payload{ { "grant_type", "client_credentials" }, { "token_type", "eg1" } }
                );
            }
        );
    }

    Response<Responses::OAuthToken> EpicClient::DeviceAuth(const cpr::Authentication& AuthClient, const std::string& AccountId, const std::string& DeviceId, const std::string& Secret)
    {
        return Call<Responses::OAuthToken, 200>(
            [&]() {
                return Http::Post(
                    Http::FormatUrl<Host::Account>("oauth/token"),
                    AuthClient,
                    cpr::Payload{ { "grant_type", "device_auth" }, {"token_type", "eg1"}, { "account_id", AccountId }, { "device_id", DeviceId }, { "secret", Secret } }
                );
            }
        );
    }

    Response<Responses::OAuthToken> EpicClient::ExchangeCode(const cpr::Authentication& AuthClient, const std::string& Code)
    {
        return Call<Responses::OAuthToken, 200>(
            [&]() {
                return Http::Post(
                    Http::FormatUrl<Host::Account>("oauth/token"),
                    AuthClient,
                    cpr::Payload{ { "grant_type", "exchange_code" }, { "token_type", "eg1" }, { "exchange_code", Code } }
                );
            }
        );
    }

    Response<Responses::OAuthToken> EpicClient::RefreshToken(const cpr::Authentication& AuthClient, const std::string& Token)
    {
        return Call<Responses::OAuthToken, 200>(
            [&]() {
                return Http::Post(
                    Http::FormatUrl<Host::Account>("oauth/token"),
                    AuthClient,
                    cpr::Payload{ { "grant_type", "refresh_token" }, { "token_type", "eg1" }, { "refresh_token", Token } }
                );
            }
        );
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