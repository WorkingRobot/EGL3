#pragma once

#include "../../MountedArchive.h"
#include "../base/Protocol.h"

#include <future>
#include <shared_mutex>
#include <string>

namespace EGL3::Service::Pipe {
    class Server {
    public:
        Server(const char* Name = Pipe::PipeName);

        ~Server();

    private:
        static constexpr uint32_t BufferSize = 4096;

        void HandleConnectionThread();

        void HandleConnection(void* PipeHandle);

        template<PacketId Id>
        void HandlePacket(const Packet<Id, PacketDirection::ServerBound>& Input, Packet<Id, PacketDirection::ClientBound>& Output);

        template<PacketId Id>
        void HandlePacket(const char Input[BufferSize], PacketHeader InputHeader, char Output[BufferSize], uint32_t& OutputSize);

        void HandlePacket(const char Input[BufferSize], uint32_t InputSize, char Output[BufferSize], uint32_t& OutputSize);

        std::weak_ptr<MountedArchive> GetMountWeak(MountHandle MountHandle);

        std::shared_ptr<MountedArchive> GetMount(MountHandle MountHandle);

        std::string PipeName;
        std::future<void> ConnectionThread;
        std::vector<std::future<void>> ConnectedThreads;
        std::vector<std::weak_ptr<MountedArchive>> Mounts;
        std::shared_mutex MountsMtx;
    };
}