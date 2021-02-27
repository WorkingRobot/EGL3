#pragma once

#include "../MountedArchive.h"
#include "Protocol.h"

#include <deque>
#include <future>
#include <string>

namespace EGL3::Service::Pipe {
    class Server {
    public:
        Server(const char* Name = Pipe::PipeName);

        ~Server();

    private:
        void HandleConnectionThread();

        void HandleConnection(void* PipeHandle);

        template<MessageType Type>
        void HandleRequest(const typename Request<Type>& Input, typename Response<Type>& Output);

        template<MessageType Type>
        void HandleRequestTyped(const std::unique_ptr<char[]>& Input, uint32_t InputSize, std::unique_ptr<char[]>& Output, uint32_t& OutputSize);

        void HandleRequest(const std::unique_ptr<char[]>& Input, uint32_t InputSize, std::unique_ptr<char[]>& Output, uint32_t& OutputSize);

        static constexpr uint32_t BufferSize = 4096;

        std::string PipeName;
        std::future<void> ConnectionThread;
        std::vector<std::future<void>> ConnectedThreads;
        std::vector<std::unique_ptr<MountedArchive>> Mounts;
    };
}