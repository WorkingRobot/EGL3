#pragma once

#include "../../utils/Callback.h"
#include "../../utils/TaskPool.h"
#include "../../storage/models/InstalledGame.h"
#include "../../web/epic/bps/ChunkData.h"
#include "../../web/epic/bps/Manifest.h"
#include "../../web/Response.h"
#include "../game/ArchiveList.h"
#include "../game/GameId.h"
#include "DownloadInfoStats.h"

#include <functional>
#include <future>
#include <mutex>
#include <sigc++/sigc++.h>
#include <variant>

namespace EGL3::Storage::Models {
    class DownloadInfo {
        constexpr static int WorkerCount = 30;

    public:
        struct StateOptions {
            std::filesystem::path DefaultArchivePath;
            std::filesystem::path ArchivePath;
            bool AutoUpdate;
            bool CreateShortcut;
        };

        struct StateInitializing {

        };

        struct StateInstalling {
            std::mutex DataMutex;
            bool Cancelled;
            std::vector<std::reference_wrapper<const Web::Epic::BPS::ChunkInfo>> UpdatedChunks;
            std::vector<uint32_t> DeletedChunkIdxs;

            std::string CloudDir;
            Web::Epic::BPS::Manifest Manifest;
            Game::Archive& Archive;

            Utils::TaskPool Pool;

            // For stats
            uint32_t PiecesTotal;
            uint64_t DownloadTotal; // In the end, this is how much should be downloaded

            std::atomic<uint32_t> PiecesComplete;
            std::atomic<uint64_t> BytesDownloadTotal;
            std::atomic<uint64_t> BytesReadTotal;
            std::atomic<uint64_t> BytesWriteTotal;

            // Used to calculate rates
            uint64_t BytesDownloadTotalLast;
            uint64_t BytesReadTotalLast;
            uint64_t BytesWriteTotalLast;

            std::mutex ChunkDataMutex;
            std::mutex ChunkInfoMutex;
            Game::ArchiveList<Game::RunlistId::File> ArchiveFiles;
            Game::ArchiveList<Game::RunlistId::ChunkPart> ArchiveChunkParts;
            Game::ArchiveList<Game::RunlistId::ChunkInfo> ArchiveChunkInfos;
            Game::ArchiveList<Game::RunlistId::ChunkData> ArchiveChunkDatas;

            StateInstalling(std::string&& CloudDir, Web::Epic::BPS::Manifest&& Manifest, Game::Archive& Archive, std::vector<std::reference_wrapper<const Web::Epic::BPS::ChunkInfo>>&& UpdatedChunks, std::vector<uint32_t>&& DeletedChunkIdxs) :
                Cancelled(false),
                CloudDir(std::move(CloudDir)),
                Manifest(std::move(Manifest)),
                Archive(Archive),
                UpdatedChunks(std::move(UpdatedChunks)),
                DeletedChunkIdxs(std::move(DeletedChunkIdxs)),
                Pool(WorkerCount),
                PiecesTotal(this->UpdatedChunks.size()),
                DownloadTotal(std::accumulate(this->UpdatedChunks.begin(), this->UpdatedChunks.end(), 0ull, [](uint64_t Val, const auto& Chunk) { return Val + Chunk.get().FileSize; })),
                PiecesComplete(0),
                BytesDownloadTotal(0),
                BytesReadTotal(0),
                BytesWriteTotal(0),
                BytesDownloadTotalLast(0),
                BytesReadTotalLast(0),
                BytesWriteTotalLast(0),
                ArchiveFiles(Archive),
                ArchiveChunkParts(Archive),
                ArchiveChunkInfos(Archive),
                ArchiveChunkDatas(Archive)
            {

            }
        };

        struct StateCancelled {

        };

        using InstalledGamesRequest = std::function<std::vector<InstalledGame>&()>;
        using LatestManifestRequest = std::function<Web::Response<Web::Epic::BPS::Manifest>(Game::GameId Id, std::string& CloudDir)>;

        DownloadInfo(Game::GameId Id, const InstalledGamesRequest& GetInstalledGames);

        ~DownloadInfo();

        DownloadInfoState GetState() const;

        void SetState(DownloadInfoState NewState);

        template<class T>
        const T& GetStateData() const {
            return std::get<T>(StateData);
        }

        template<class T>
        T& GetStateData() {
            return std::get<T>(StateData);
        }

        void CancelDownloadSetup();

        void BeginDownload(const LatestManifestRequest& GetLatestManifest);

        void SetDownloadRunning(bool Running = true);

        void SetDownloadCancelled();

        bool InstallOne();

        void ReplaceInstallOne(const Web::Epic::BPS::ChunkInfo& Chunk, uint32_t ReplaceIdx);

        void AppendInstallOne(const Web::Epic::BPS::ChunkInfo& Chunk);

        Utils::Callback<void(const DownloadInfoStats&)> OnStatsUpdate;

        sigc::signal<void(DownloadInfoState)> OnStateUpdate;

    private:
        Web::Response<Web::Epic::BPS::ChunkData> GetChunk(const Web::Epic::BPS::ChunkInfo& Chunk) const;

        // Returns the data sector to use for the chunk data runlist
        uint32_t AllocateChunkData(uint32_t WindowSize);

        static void InitializeArchive(Game::GameId Id, const Web::Epic::BPS::ManifestMeta& Meta, const std::string& CloudDir, Game::ArchiveRef<Game::Header> Header, Game::ArchiveRef<Game::ManifestData> ManifestData);

        Game::GameId Id;
        InstalledGame* GameConfig;

        DownloadInfoState CurrentState;
        std::variant<StateOptions, StateInitializing, StateInstalling, StateCancelled> StateData;
        std::future<void> PrimaryTask;

        const InstalledGamesRequest GetInstalledGames;
    };
}