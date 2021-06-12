#pragma once

#include "../../storage/game/GameId.h"
#include "../../web/epic/bps/ChunkData.h"
#include "../../web/epic/bps/Manifest.h"
#include "../../web/Response.h"

namespace EGL3::Utils::EGL {
    class ChunkProvider {
    public:
        ChunkProvider(Storage::Game::GameId Id);

        bool IsValid() const;

        const std::filesystem::path& GetInstallLocation() const;

        bool IsChunkProbablyAvailable(const Utils::Guid& Guid) const;

        std::unique_ptr<char[]> GetChunk(const Utils::Guid& Guid) const;

    private:
        static std::filesystem::path GetInstalledItemFolderPath();

        void SetupLUT(Web::Epic::BPS::Manifest&& Manifest);

        struct ChunkSection {
            const std::string* File;
            size_t FileOffset;
            uint32_t ChunkOffset;
            uint32_t Size;
        };

        struct ChunkData {
            std::vector<ChunkSection> Sections;
            const Web::Epic::BPS::ChunkInfo Info;
        };

        std::filesystem::path InstallLocation;
        std::vector<Web::Epic::BPS::ChunkInfo> ChunkInfos;
        std::vector<std::string> Filenames;
        std::unordered_map<Utils::Guid, ChunkData> LUT;

    };
}
