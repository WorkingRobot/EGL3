#pragma once

#include "../base/Protocol.h"

#include <filesystem>
#include <string>

namespace EGL3::Service::Pipe {
    class Client {
    public:
        Client();
        ~Client();

        bool Connect(const std::string& ClientName, const char* Name = PipeName);
        bool IsConnected() const;

        const std::string& GetServerName() const;

        PacketResponse BeginMount(const std::filesystem::path& Path, MountHandle& Handle);
        PacketResponse GetMountPath(MountHandle Handle, std::filesystem::path& Path);
        PacketResponse GetMountInfo(MountHandle Handle);
        PacketResponse EndMount(MountHandle Handle);

    private:
        PacketResponse Handshake(PacketProtocol Protocol, const std::string& ClientName, std::string& ServerName);

        template<PacketId Id>
        bool Transact(const Packet<Id, PacketDirection::ServerBound>& Request, Packet<Id, PacketDirection::ClientBound>& Response);

        static constexpr uint16_t BufferSize = 4096;

        void* PipeHandle;
        bool Connected;

        std::string ServerName;
    };
}