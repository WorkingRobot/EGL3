#include "LauncherContentClient.h"

namespace EGL3::Web::Epic {
    LauncherContentClient::LauncherContentClient(EpicClientAuthed& BaseClient) :
        BaseClient(BaseClient)
    {

    }

    const BPS::Manifest& LauncherContentClient::GetManifest()
    {
        if (CurrentManifest.has_value()) {
            return CurrentManifest.value();
        }

        auto Info = BaseClient.GetLauncherDownloadInfo("Windows", "Live-EternalKnight");
        EGL3_CONDITIONAL_LOG(!Info.HasError(), LogLevel::Critical, "Could not get laucher download info");

        auto ContentInfoPtr = Info->GetElement("EpicGamesLauncherContent");
        EGL3_CONDITIONAL_LOG(ContentInfoPtr, LogLevel::Critical, "Could not get LauncherContent element");

        auto& ManifestInfo = ContentInfoPtr->PickManifest();
        auto Manifest = Client.GetManifest(ManifestInfo);
        EGL3_CONDITIONAL_LOG(!Manifest.HasError(), LogLevel::Critical, "Could not download LauncherContent manifest");
        EGL3_CONDITIONAL_LOG(!Manifest->HasError(), LogLevel::Critical, "Could not parse LauncherContent manifest");

        CloudDir = ManifestInfo.GetCloudDir();
        return CurrentManifest.emplace(std::move(Manifest.Get()));
    }

    LauncherContentStream* LauncherContentClient::GetFile(const std::string& File)
    {
        auto Itr = OpenedStreams.find(File);
        if (Itr != OpenedStreams.end()) {
            return &Itr->second;
        }
        else {
            auto& Manifest = GetManifest();
            auto FilePtr = Manifest.GetFile(File);
            if (FilePtr) {
                return &OpenedStreams.emplace(File, LauncherContentStream(Manifest, *FilePtr, [this](const Utils::Guid& Guid) { return GetChunkData(Guid); })).first->second;
            }
            else {
                return nullptr;
            }
        }
    }

    const std::unique_ptr<char[]>* LauncherContentClient::GetChunkData(const Utils::Guid& Guid)
    {
        auto Itr = DownloadedChunks.find(Guid);
        if (Itr != DownloadedChunks.end()) {
            return &Itr->second;
        }
        else {
            auto& Manifest = GetManifest();
            auto InfoPtr = Manifest.GetChunk(Guid);
            if (InfoPtr) {
                auto Chunk = Client.GetChunk(CloudDir.value(), Manifest.ManifestMeta.FeatureLevel, *InfoPtr);
                EGL3_CONDITIONAL_LOG(!Chunk.HasError(), LogLevel::Critical, "Could not download chunk");
                EGL3_CONDITIONAL_LOG(!Chunk->HasError(), LogLevel::Critical, "Could not parse chunk");
                return &DownloadedChunks.emplace(Guid, std::move(Chunk->Data)).first->second;
            }
            else {
                return nullptr;
            }
        }
    }
}