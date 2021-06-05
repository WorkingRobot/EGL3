#include "LauncherContentClient.h"

namespace EGL3::Web::Epic::Content {
    LauncherContentClient::LauncherContentClient(EpicClientAuthed& BaseClient, const std::filesystem::path& CacheDir) :
        BaseClient(BaseClient),
        CacheDir(CacheDir)
    {

    }

    const BPS::Manifest& LauncherContentClient::GetManifest()
    {
        if (CurrentManifest.has_value()) {
            return CurrentManifest.value();
        }

        auto Info = BaseClient.GetLauncherDownloadInfo("Windows", "Live-EternalKnight");
        EGL3_VERIFY(!Info.HasError(), "Could not get launcher download info");

        auto ContentInfoPtr = Info->GetElement("EpicGamesLauncherContent");
        EGL3_VERIFY(ContentInfoPtr, "Could not get LauncherContent element");

        auto Manifest = Client.GetManifestCacheable(*ContentInfoPtr, CloudDir, CacheDir);
        EGL3_VERIFY(!Manifest.HasError(), "Could not download LauncherContent manifest");
        EGL3_VERIFY(!Manifest->HasError(), "Could not parse LauncherContent manifest");

        return CurrentManifest.emplace(std::move(Manifest.Get()));
    }

    const char* LauncherContentClient::GetChunkData(const Utils::Guid& Guid)
    {
        auto Itr = DownloadedChunks.find(Guid);
        if (Itr != DownloadedChunks.end()) {
            return Itr->second.get();
        }
        else {
            auto& Manifest = GetManifest();
            auto InfoPtr = Manifest.GetChunk(Guid);
            if (InfoPtr) {
                auto Chunk = Client.GetChunkCacheable(CloudDir, Manifest.ManifestMeta.FeatureLevel, *InfoPtr, CacheDir);
                EGL3_VERIFY(!Chunk.HasError(), "Could not download chunk");
                EGL3_VERIFY(!Chunk->HasError(), "Could not parse chunk");
                return DownloadedChunks.emplace(Guid, std::move(Chunk->Data)).first->second.get();
            }
            else {
                return nullptr;
            }
        }
    }

    bool LauncherContentClient::GetFile(const BPS::FileManifest& File, LauncherContentStream& Out)
    {
        Out = LauncherContentStream(GetManifest(), File, [this](const Utils::Guid& Guid) { return GetChunkData(Guid); });
        return true;
    }

    bool LauncherContentClient::GetFile(const std::string& File, LauncherContentStream& Out)
    {
        if (auto FilePtr = GetManifest().GetFile(File)) {
            return GetFile(*FilePtr, Out);
        }
        else {
            return false;
        }
    }

    bool LauncherContentClient::GetJson(const BPS::FileManifest& File, rapidjson::Document& Out)
    {
        LauncherContentStream FileStream;
        if (!GetFile(File, FileStream)) {
            return false;
        }

        FileStream.seek(0, Utils::Streams::Stream::Beg);
        auto Data = std::make_unique<char[]>(FileStream.size());
        FileStream.read(Data.get(), FileStream.size());
        rapidjson::MemoryStream DataStream(Data.get(), FileStream.size());
        rapidjson::EncodedInputStream<rapidjson::UTF8<>, decltype(DataStream)> JsonStream(DataStream);

        Out.ParseStream(JsonStream);
        return !Out.HasParseError();
    }

    bool LauncherContentClient::GetJson(const std::string& File, rapidjson::Document& Out)
    {
        LauncherContentStream FileStream;
        if (!GetFile(File, FileStream)) {
            return false;
        }

        FileStream.seek(0, Utils::Streams::Stream::Beg);
        auto Data = std::make_unique<char[]>(FileStream.size());
        FileStream.read(Data.get(), FileStream.size());
        rapidjson::MemoryStream DataStream(Data.get(), FileStream.size());
        rapidjson::EncodedInputStream<rapidjson::UTF8<>, decltype(DataStream)> JsonStream(DataStream);

        Out.ParseStream(JsonStream);
        return !Out.HasParseError();
    }

    const NotificationList& LauncherContentClient::GetNotifications()
    {
        if (!Notifications.has_value()) {
            rapidjson::Document NotifJson;
            EGL3_VERIFY(GetJson("BuildNotificationsV2.json", NotifJson), "No notification json file loaded");

            auto& NotifList = Notifications.emplace();
            EGL3_VERIFY(NotificationList::Parse(NotifJson, NotifList), "Notification file has bad json");
        }
        return Notifications.value();
    }

    void LauncherContentClient::LoadSdMetaData()
    {
        if (!SdMetas.has_value()) {
            SdMetas.emplace();
            auto& Manifest = GetManifest();
            for (auto& File : Manifest.FileManifestList.FileList) {
                if (File.Filename.ends_with("sdmeta")) {
                    rapidjson::Document SdJson;
                    if (!EGL3_ENSURE(GetJson(File, SdJson), LogLevel::Warning, "Could not get json from sdmeta")) {
                        continue;
                    }

                    SdMeta SdMeta;
                    if (!EGL3_ENSURE(SdMeta::Parse(SdJson, SdMeta), LogLevel::Warning, "Sdmeta has bad json")) {
                        continue;
                    }

                    SdMetas->emplace_back(std::move(SdMeta));
                }
            }
        }
    }

    const std::vector<SdMeta::Data>* LauncherContentClient::GetSdMetaData(const std::string& AppName, const std::string& Version)
    {
        LoadSdMetaData();

        auto Itr = std::find_if(SdMetas->begin(), SdMetas->end(), [&](const SdMeta& Meta) {
            return std::any_of(Meta.Builds.begin(), Meta.Builds.end(), [&](const SdMeta::Build& Build) {
                if (Build.Asset != AppName) {
                    return false;
                }
                if (Build.Version == Version) {
                    return true;
                }
                if (Build.Version == "*") {
                    return true;
                }
                return SdMetaEvaluator.Evaluate(Build.Version, Version);
            });
        });

        if (Itr != SdMetas->end()) {
            return &Itr->Datas;
        }
        return nullptr;
    }
}