#pragma once

#include "Protocol.h"

#include <filesystem>
#include <string>

namespace EGL3::Service::Pipe {
    class Client {
    public:
        Client(const char* Name = PipeName);

        ~Client();

        bool OpenArchive(const std::filesystem::path& Path, void*& Ctx);
        bool ReadArchive(void* Ctx);
        bool InitializeDisk(void* Ctx);
        bool CreateLUT(void* Ctx);
        bool CreateDisk(void* Ctx);
        bool MountDisk(void* Ctx);
        bool Destruct(void* Ctx);
        bool QueryPath(void* Ctx, std::filesystem::path& Path);
        bool QueryLetter(void* Ctx, char& Letter);
        bool QueryStats(void* Ctx, MountedArchive::Stats& Stats);

    private:
        template<MessageType Type>
        bool Transact(const typename Request<Type>& Req, typename Response<Type>& Resp);

        static constexpr uint32_t BufferSize = 4096;

        void* PipeHandle;
    };
}