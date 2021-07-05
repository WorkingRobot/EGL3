#pragma once

#include "../../utils/egl/ChunkProvider.h"
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
        constexpr static size_t WorkerCount = 32;

    public:
        struct StateOptions {
            std::filesystem::path DefaultArchivePath;
            std::filesystem::path ArchivePath;
            InstallFlags Flags;
            std::vector<std::string> SelectedIds;
            std::vector<std::string> InstallTags;

            Utils::EGL::ChunkProvider EGLProvider;

            StateOptions(Storage::Game::GameId Id) :
                EGLProvider(Id)
            {

            }
        };

        struct StateInitializing {
            std::vector<std::string> InstallTags;

            Utils::EGL::ChunkProvider EGLProvider;

            StateInitializing(std::vector<std::string>&& InstallTags, Utils::EGL::ChunkProvider&& EGLProvider) :
                InstallTags(std::move(InstallTags)),
                EGLProvider(std::move(EGLProvider))
            {

            }
        };

        struct StateInstalling {
            std::mutex DataMutex;
            bool Cancelled;
            std::vector<std::reference_wrapper<const Web::Epic::BPS::ChunkInfo>> UpdatedChunks;
            std::vector<uint32_t> DeletedChunkIdxs;

            std::string CloudDir;
            Web::Epic::BPS::Manifest Manifest;
            std::vector<std::reference_wrapper<const Web::Epic::BPS::FileManifest>> ManifestFiles;
            Game::Archive& Archive;

            Utils::TaskPool Pool;

            Utils::EGL::ChunkProvider EGLProvider;

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

            StateInstalling(std::string&& CloudDir, Web::Epic::BPS::Manifest&& Manifest, std::vector<std::reference_wrapper<const Web::Epic::BPS::FileManifest>>&& ManifestFiles, Game::Archive& Archive, std::vector<std::reference_wrapper<const Web::Epic::BPS::ChunkInfo>>&& UpdatedChunks, std::vector<uint32_t>&& DeletedChunkIdxs, Utils::EGL::ChunkProvider&& EGLProvider) :
                Cancelled(false),
                CloudDir(std::move(CloudDir)),
                Manifest(std::move(Manifest)),
                ManifestFiles(std::move(ManifestFiles)),
                Archive(Archive),
                UpdatedChunks(std::move(UpdatedChunks)),
                DeletedChunkIdxs(std::move(DeletedChunkIdxs)),
                Pool(WorkerCount),
                EGLProvider(std::move(EGLProvider)),
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

        using LatestManifestRequest = std::function<Web::Response<Web::Epic::BPS::Manifest>(Game::GameId Id, std::string& CloudDir)>;
        using CreateGameConfig = std::function<InstalledGame&()>;

        DownloadInfo(Game::GameId Id, InstalledGame* GameConfig);

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

        template<class T>
        const T* TryGetStateData() const {
            return std::get_if<T>(&StateData);
        }

        template<class T>
        T* TryGetStateData() {
            return std::get_if<T>(&StateData);
        }

        void CancelDownloadSetup();

        void BeginDownload(const LatestManifestRequest& GetLatestManifest, const CreateGameConfig& CreateGameConfig);

        void SetDownloadRunning(bool Running = true);

        void SetDownloadCancelled();

        bool InstallOne();

        void InstallOne(const Web::Epic::BPS::ChunkInfo& Chunk, Storage::Game::ChunkInfo& ChunkInfoData);

        Utils::Callback<void(const DownloadInfoStats&)> OnStatsUpdate;
        Utils::Callback<void(const Utils::Guid&, ChunkState)> OnChunkUpdate;

        sigc::signal<void(DownloadInfoState)> OnStateUpdate;

    private:
        Web::Response<Web::Epic::BPS::ChunkData> GetChunk(const Web::Epic::BPS::ChunkInfo& Chunk) const;

        Storage::Game::ChunkInfo& AllocateChunkInfo();

        // Returns the data sector to use for the chunk data runlist
        uint32_t AllocateChunkData(uint32_t WindowSize);

        static void InitializeArchive(Game::GameId Id, const Web::Epic::BPS::ManifestMeta& Meta, const std::string& CloudDir, Game::ArchiveRef<Game::Header> Header, Game::ArchiveRef<Game::ManifestData> ManifestData);

        Game::GameId Id;
        InstalledGame* GameConfig;

        DownloadInfoState CurrentState;
        std::variant<StateOptions, StateInitializing, StateInstalling, StateCancelled> StateData;
        std::future<void> PrimaryTask;
    };
}