#pragma once

#include "../EpicClient.h"
#include "../EpicClientAuthed.h"
#include "LauncherContentStream.h"

#include <optional>

namespace EGL3::Web::Epic {
    // Allows access to files from the "EpicGamesLauncherContent" app repository
    // Treat it like an "extension" to EpicClientAuthed
    // This allows the use of the same token and adds extensibility
    class LauncherContentClient {
    public:
        LauncherContentClient(EpicClientAuthed& BaseClient);

        const BPS::Manifest& GetManifest();

        LauncherContentStream* GetFile(const std::string& File);

        const std::unique_ptr<char[]>* GetChunkData(const Utils::Guid& Guid);

    private:
        mutable std::optional<BPS::Manifest> CurrentManifest;
        mutable std::optional<std::string> CloudDir;
        std::unordered_map<std::string, LauncherContentStream> OpenedStreams;
        std::unordered_map<Utils::Guid, std::unique_ptr<char[]>> DownloadedChunks;

        EpicClientAuthed& BaseClient;
        EpicClient Client;
    };
}