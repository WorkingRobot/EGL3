#pragma once

#include "../../../utils/stringex/ExpressionEvaluator.h"
#include "../EpicClient.h"
#include "../EpicClientAuthed.h"
#include "LauncherContentStream.h"
#include "NotificationList.h"
#include "SdMeta.h"

#include <optional>

namespace EGL3::Web::Epic::Content {
    // Allows access to files from the "EpicGamesLauncherContent" app repository
    // Treat it like an "extension" to EpicClientAuthed
    // This allows the use of the same token and adds extensibility
    class LauncherContentClient {
    public:
        LauncherContentClient(EpicClientAuthed& BaseClient, const std::filesystem::path& CacheDir = "");

        const BPS::Manifest& GetManifest();

        const char* GetChunkData(const Utils::Guid& Guid);

        bool GetFile(const BPS::FileManifest& File, LauncherContentStream& Out);

        bool GetFile(const std::string& File, LauncherContentStream& Out);

        bool GetJson(const BPS::FileManifest& File, rapidjson::Document& Out);

        bool GetJson(const std::string& File, rapidjson::Document& Out);

        const NotificationList& GetNotifications();

        void LoadSdMetaData();

        const std::vector<SdMeta::Data>* GetSdMetaData(const std::string& CatalogItemId, const std::string& AppName, const std::string& Version);

    private:
        std::optional<BPS::Manifest> CurrentManifest;
        std::string CloudDir;

        std::unordered_map<Utils::Guid, std::unique_ptr<char[]>> DownloadedChunks;

        std::optional<NotificationList> Notifications;

        std::optional<std::vector<SdMeta>> SdMetas;
        Utils::StringEx::ExpressionEvaluator SdMetaEvaluator;

        EpicClientAuthed& BaseClient;
        EpicClient Client;
        std::filesystem::path CacheDir;
    };
}