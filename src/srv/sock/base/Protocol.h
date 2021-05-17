#pragma once

#include "../../../storage/game/ChunkInfo.h"
#include "../../../storage/game/ChunkPart.h"
#include "../../../storage/game/File.h"
#include "../../../storage/game/GameId.h"
#include "../../../storage/game/ManifestData.h"
#include "../../../storage/game/UpdateInfo.h"
#include "../../../storage/models/legacy/InstalledGame.h"
#include "../../../utils/streams/Stream.h"
#include "../../../utils/Guid.h"

#include <memory>

namespace EGL3::Service::Sock {
    enum class PacketId : uint16_t {
        Handshake,
        TransferInstalledGames,
        GetPreInstallInfo,
        SetPreInstallInfo,
        BeginUpdate,
        GetInstalledChunkGuids,
        GetUpdateInfo,
        SetUpdateInfo,
        IncrementUpdateInfo,
        InitializeUpdate,
        SetFiles,
        ReplaceChunk,
        AppendChunk,
        DeleteChunks,
        SetChunkPartIndexes,
        EndUpdate,
        BeginMount,
        GetMountPath,
        GetMountInfo,
        EndMount,
    };

    enum class PacketDirection : bool {
        ClientBound = false,
        ServerBound = true
    };

    enum class PacketProtocol : uint16_t {
        Initial     = 0x0000
    };

    enum class PacketResponse : uint16_t {
        Success
    };

    class PacketHeader {
        // XX XX YY YY YY YY YY YY
        // Id    Size
        uint64_t Data;

    public:
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, PacketHeader& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const PacketHeader& Val);

        uint64_t GetSize() const;

        void SetSize(uint64_t Val);

        uint16_t GetId() const;

        void SetId(uint16_t Val);
    };

    template<PacketId Id, PacketDirection Direction>
    class PacketData {
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, PacketData& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const PacketData& Val);
    };

    template<>
    class PacketData<PacketId::Handshake, PacketDirection::ServerBound> {
        PacketProtocol Protocol;
        std::string ClientName;
    };

    template<>
    class PacketData<PacketId::Handshake, PacketDirection::ClientBound> {
        PacketResponse Response;
        std::string ServerName;
    };

    template<>
    class PacketData<PacketId::TransferInstalledGames, PacketDirection::ServerBound> {
        std::vector<Storage::Models::Legacy::InstalledGame> Games;
    };

    template<>
    class PacketData<PacketId::TransferInstalledGames, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::GetPreInstallInfo, PacketDirection::ServerBound> {
        Storage::Game::GameId Id;
    };

    template<>
    class PacketData<PacketId::GetPreInstallInfo, PacketDirection::ClientBound> {
        PacketResponse Response;
        std::filesystem::path Path;
        std::vector<std::string> InstalledComponents;
        bool AutoUpdate;
        bool CreateShortcut;
    };

    template<>
    class PacketData<PacketId::SetPreInstallInfo, PacketDirection::ServerBound> {
        Storage::Game::GameId Id;
        std::filesystem::path Path;
        std::vector<std::string> InstalledComponents;
        bool AutoUpdate;
        bool CreateShortcut;
    };

    template<>
    class PacketData<PacketId::SetPreInstallInfo, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::BeginUpdate, PacketDirection::ServerBound> {
        Storage::Game::GameId Id;
    };

    template<>
    class PacketData<PacketId::BeginUpdate, PacketDirection::ClientBound> {
        PacketResponse Response;
        uint32_t UpdateHandle;
    };

    template<>
    class PacketData<PacketId::GetInstalledChunkGuids, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
    };

    template<>
    class PacketData<PacketId::GetInstalledChunkGuids, PacketDirection::ClientBound> {
        PacketResponse Response;
        std::vector<Utils::Guid> Guids;
    };

    template<>
    class PacketData<PacketId::GetUpdateInfo, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
    };

    template<>
    class PacketData<PacketId::GetUpdateInfo, PacketDirection::ClientBound> {
        PacketResponse Response;
        Storage::Game::UpdateInfo UpdateInfo;
    };

    template<>
    class PacketData<PacketId::SetUpdateInfo, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        Storage::Game::UpdateInfo UpdateInfo;
    };

    template<>
    class PacketData<PacketId::SetUpdateInfo, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::IncrementUpdateInfo, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        uint64_t NanosecondsElapsed;
        uint32_t PiecesComplete;
        uint64_t BytesDownloadTotal;
    };

    template<>
    class PacketData<PacketId::IncrementUpdateInfo, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::InitializeUpdate, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        std::string GameName;
        std::string VersionLong;
        std::string VersionHR;
        uint64_t VersionNum;
        Storage::Game::ManifestData ManifestData;
    };

    template<>
    class PacketData<PacketId::InitializeUpdate, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::SetFiles, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        std::vector<Storage::Game::File> Files;
        std::vector<Storage::Game::ChunkPart> ChunkParts;
    };

    template<>
    class PacketData<PacketId::SetFiles, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::ReplaceChunk, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        uint32_t ReplaceIdx;
        Storage::Game::ChunkInfo Info;
        std::unique_ptr<char[]> Data;
    };

    template<>
    class PacketData<PacketId::ReplaceChunk, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::AppendChunk, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        Storage::Game::ChunkInfo Info;
        std::unique_ptr<char[]> Data;
    };

    template<>
    class PacketData<PacketId::AppendChunk, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::DeleteChunks, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        std::vector<uint32_t> IdxsToDelete;
    };

    template<>
    class PacketData<PacketId::DeleteChunks, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::SetChunkPartIndexes, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
        std::vector<std::vector<uint32_t>> ChunkIdxs;
    };

    template<>
    class PacketData<PacketId::SetChunkPartIndexes, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::EndUpdate, PacketDirection::ServerBound> {
        uint32_t UpdateHandle;
    };

    template<>
    class PacketData<PacketId::EndUpdate, PacketDirection::ClientBound> {
        PacketResponse Response;
    };

    template<>
    class PacketData<PacketId::BeginMount, PacketDirection::ServerBound> {
        Storage::Game::GameId Id;
    };

    template<>
    class PacketData<PacketId::BeginMount, PacketDirection::ClientBound> {
        PacketResponse Response;
        uint32_t MountHandle;
    };

    template<>
    class PacketData<PacketId::GetMountPath, PacketDirection::ServerBound> {
        uint32_t MountHandle;
    };

    template<>
    class PacketData<PacketId::GetMountPath, PacketDirection::ClientBound> {
        PacketResponse Response;
        std::filesystem::path Path;
    };

    template<>
    class PacketData<PacketId::GetMountInfo, PacketDirection::ServerBound> {
        uint32_t MountHandle;
    };

    template<>
    class PacketData<PacketId::GetMountInfo, PacketDirection::ClientBound> {
        PacketResponse Response;
        // No info yet
    };

    template<>
    class PacketData<PacketId::EndMount, PacketDirection::ServerBound> {
        uint32_t MountHandle;
    };

    template<>
    class PacketData<PacketId::EndMount, PacketDirection::ClientBound> {
        PacketResponse Response;
    };
}