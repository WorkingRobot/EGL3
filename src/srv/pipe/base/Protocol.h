#pragma once

#include "../../../storage/game/GameId.h"
#include "../../../utils/streams/Stream.h"

#include <filesystem>

namespace EGL3::Service::Pipe {
    static constexpr const char* PipeName = R"(\\.\pipe\EGL3Launcher)";

    enum class PacketId : uint16_t {
        Handshake,
        BeginMount,
        GetMountPath,
        GetMountInfo,
        EndMount
    };

    enum class PacketDirection : bool {
        ClientBound = false,
        ServerBound = true
    };
    
    enum class PacketProtocol : uint16_t {
        Initial  = 0x0000,

        LatestPlusOne,
        Latest = LatestPlusOne - 1,

        Outdated = 0xFFFF,
    };

    enum class PacketResponse : uint16_t {
        Success,
        ServerOutdated,
        ClientOutdated,
        InvalidArchive,
        ArchiveAlreadyExists,
        InvalidMountHandle,
        NoMountPathForArchiveYet,
        ArchiveStillExists,

        TransactionFailure = 0xFFFF
    };

    class PacketHeader {
        // XX XX YY YY YY YY YY YY
        // Id    Size
        uint64_t Data;

    public:
        PacketHeader(PacketId Id, uint64_t Size) :
            Data((Size << 0) | (uint64_t(Id) << 48))
        {

        }

        uint64_t GetSize() const
        {
            return (Data & 0x0000FFFFFFFFFFFFllu) >> 0;
        }

        void SetSize(uint64_t Val)
        {
            Data = (Val << 0) | (uint64_t(GetId()) << 48);
        }

        PacketId GetId() const
        {
            return PacketId(Data >> 48);
        }

        void SetId(PacketId Val)
        {
            Data = (GetSize() << 0) | (uint64_t(Val) << 48);
        }
    };

    using MountHandle = void*;

    static Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, MountHandle& Val)
    {
        Stream >> (size_t&)Val;

        return Stream;
    }

    static Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const MountHandle& Val)
    {
        Stream << (size_t)Val;

        return Stream;
    }

    template<PacketId Id, PacketDirection Direction>
    struct Packet {
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, Packet& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const Packet& Val);
    };

    template<>
    struct Packet<PacketId::Handshake, PacketDirection::ServerBound> {
        PacketProtocol Protocol;
        std::string ClientName;
    };

    template<>
    struct Packet<PacketId::Handshake, PacketDirection::ClientBound> {
        PacketResponse Response;
        std::string ServerName;
    };

    template<>
    struct Packet<PacketId::BeginMount, PacketDirection::ServerBound> {
        std::filesystem::path Path;
    };

    template<>
    struct Packet<PacketId::BeginMount, PacketDirection::ClientBound> {
        PacketResponse Response;
        MountHandle MountHandle;
    };

    template<>
    struct Packet<PacketId::GetMountPath, PacketDirection::ServerBound> {
        MountHandle MountHandle;
    };

    template<>
    struct Packet<PacketId::GetMountPath, PacketDirection::ClientBound> {
        PacketResponse Response;
        std::filesystem::path Path;
    };

    template<>
    struct Packet<PacketId::GetMountInfo, PacketDirection::ServerBound> {
        MountHandle MountHandle;
    };

    template<>
    struct Packet<PacketId::GetMountInfo, PacketDirection::ClientBound> {
        PacketResponse Response;
        // No info yet
    };

    template<>
    struct Packet<PacketId::EndMount, PacketDirection::ServerBound> {
        MountHandle MountHandle;
    };

    template<>
    struct Packet<PacketId::EndMount, PacketDirection::ClientBound> {
        PacketResponse Response;
    };
}

#include "Protocol.inl"