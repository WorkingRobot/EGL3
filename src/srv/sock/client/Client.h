#pragma once

#include "../base/Protocol.h"

namespace EGL3::Service::Sock {
    class Client {
    public:
        Client();

        ~Client();

        PacketResponse TransferGames(const std::vector<Storage::Models::Legacy::InstalledGame>& Games);

        PacketResponse GetPreInstallInfo(Storage::Game::GameId Id, std::filesystem::path& Path, std::vector<std::string>& InstalledComponents, bool& AutoUpdate, bool& CreateShortcut);

        PacketResponse SetPreInstallInfo(Storage::Game::GameId Id, const std::filesystem::path& Path, const std::vector<std::string>& InstalledComponents, bool AutoUpdate, bool CreateShortcut);

        PacketResponse BeginUpdate(Storage::Game::GameId Id, uint32_t& UpdateHandle);

        PacketResponse GetInstalledChunkGuids(uint32_t UpdateHandle, std::vector<Utils::Guid>& Guids);

        PacketResponse GetUpdateInfo(uint32_t UpdateHandle, Storage::Game::UpdateInfo& UpdateInfo);

        PacketResponse SetUpdateInfo(uint32_t UpdateHandle, const Storage::Game::UpdateInfo& UpdateInfo);

        PacketResponse IncrementUpdateInfo(uint32_t UpdateHandle, uint64_t NanosecondsElapsed, uint64_t PiecesComplete, uint64_t BytesDownloadTotal);

        PacketResponse InitializeUpdate(uint32_t UpdateHandle, const std::string& GameName, const std::string& VersionLong, const std::string& VersionHR, uint64_t VersionNum, const Storage::Game::ManifestData& ManifestData);

        PacketResponse SetFiles(uint32_t UpdateHandle, const std::vector<Storage::Game::File>& Files, const std::vector<Storage::Game::ChunkPart>& ChunkParts);

        PacketResponse ReplaceChunk(uint32_t UpdateHandle, uint32_t ReplaceIdx, const Storage::Game::ChunkInfo& Info, const std::unique_ptr<char[]>& Data);

        PacketResponse AppendChunk(uint32_t UpdateHandle, const Storage::Game::ChunkInfo& Info, const std::unique_ptr<char[]>& Data);

        PacketResponse DeleteChunks(uint32_t UpdateHandle, const std::vector<uint32_t>& IdxsToDelete);

        PacketResponse SetChunkPartIndexes(uint32_t UpdateHandle, const std::vector<std::vector<uint32_t>>& ChunkIdxs);

        PacketResponse EndUpdate(uint32_t UpdateHandle);

        PacketResponse BeginMount(Storage::Game::GameId Id, uint32_t& MountHandle);

        PacketResponse GetMountPath(uint32_t MountHandle, std::filesystem::path& Path);

        PacketResponse GetMountInfo(uint32_t MountHandle);

        PacketResponse EndMount(uint32_t MountHandle);

        const std::string& GetServerName();

    private:
        std::string ServerName;
    };
}