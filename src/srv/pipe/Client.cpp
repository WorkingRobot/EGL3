#include "Client.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Service::Pipe {
    Client::Client(const char* Name) :
        Connected(false)
    {
        while (true) {
            PipeHandle = CreateFile(Name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

            if (PipeHandle != INVALID_HANDLE_VALUE) {
                break;
            }

            if (GetLastError() != ERROR_PIPE_BUSY) {
                printf("CreateFile failed, GLE=%d.\n", GetLastError());
                return;
            }

            if (!WaitNamedPipe(Name, 1000)) {
                printf("WaitNamedPipe timed out, GLE=%d.\n", GetLastError());
                return;
            }
        }

        DWORD PipeMode = PIPE_READMODE_MESSAGE;
        bool Success = SetNamedPipeHandleState(PipeHandle, &PipeMode, NULL, NULL);
        if (!Success) {
            printf("SetNamedPipeHandleState failed, GLE=%d.\n", GetLastError());
            return;
        }

        Connected = true;
    }

    Client::~Client()
    {
        CloseHandle(PipeHandle);
    }

    bool Client::IsConnected() const
    {
        return Connected;
    }

    bool Client::OpenArchive(const std::filesystem::path& Path, void*& Ctx)
    {
        Request<MessageType::OpenArchive> Req{};
        Response<MessageType::OpenArchive> Resp{};

        wcscpy_s(Req.FilePath, Path.c_str());

        if (!Transact(Req, Resp)) {
            return false;
        }

        if (Resp.Status != ResponseStatus::Success) {
            return false;
        }

        Ctx = Resp.Context;
        return true;
    }

    bool Client::ReadArchive(void* Ctx)
    {
        Request<MessageType::ReadArchive> Req{ .Context = Ctx };
        Response<MessageType::ReadArchive> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        if (Resp.Status != ResponseStatus::Success) {
            return false;
        }

        return true;
    }

    bool Client::InitializeDisk(void* Ctx)
    {
        Request<MessageType::InitializeDisk> Req{ .Context = Ctx };
        Response<MessageType::InitializeDisk> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        if (Resp.Status != ResponseStatus::Success) {
            return false;
        }

        return true;
    }

    bool Client::CreateLUT(void* Ctx)
    {
        Request<MessageType::CreateLUT> Req{ .Context = Ctx };
        Response<MessageType::CreateLUT> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        if (Resp.Status != ResponseStatus::Success) {
            return false;
        }

        return true;
    }

    bool Client::CreateDisk(void* Ctx)
    {
        Request<MessageType::CreateDisk> Req{ .Context = Ctx };
        Response<MessageType::CreateDisk> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        if (Resp.Status != ResponseStatus::Success) {
            return false;
        }

        return true;
    }

    bool Client::MountDisk(void* Ctx)
    {
        Request<MessageType::MountDisk> Req{ .Context = Ctx };
        Response<MessageType::MountDisk> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        if (Resp.Status != ResponseStatus::Success) {
            return false;
        }

        return true;
    }

    bool Client::Destruct(void* Ctx)
    {
        Request<MessageType::Destruct> Req{ .Context = Ctx };
        Response<MessageType::Destruct> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        if (Resp.Status != ResponseStatus::Success) {
            return false;
        }

        return true;
    }

    bool Client::QueryPath(void* Ctx, std::filesystem::path& Path)
    {
        Request<MessageType::QueryPath> Req{ .Context = Ctx };
        Response<MessageType::QueryPath> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        Path = Resp.FilePath;
        return true;
    }

    bool Client::QueryLetter(void* Ctx, char& Letter)
    {
        Request<MessageType::QueryLetter> Req{ .Context = Ctx };
        Response<MessageType::QueryLetter> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        Letter = Resp.Letter;
        return true;
    }

    bool Client::QueryStats(void* Ctx, MountedArchive::Stats& Stats)
    {
        Request<MessageType::QueryStats> Req{ .Context = Ctx };
        Response<MessageType::QueryStats> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        Stats = Resp.Stats;
        return true;
    }

    bool Client::QueryServer(uint32_t& ProtocolVersion, uint32_t& MountedDiskCount)
    {
        Request<MessageType::QueryServer> Req{};
        Response<MessageType::QueryServer> Resp{};

        if (!Transact(Req, Resp)) {
            return false;
        }

        ProtocolVersion = Resp.ProtocolVersion;
        MountedDiskCount = Resp.MountedDiskCount;
        return true;
    }

    template<MessageType Type>
    bool Client::Transact(const typename Request<Type>& Req, typename Response<Type>& Resp)
    {
        DWORD BytesRead = 0;
        bool Success = TransactNamedPipe(PipeHandle, (void*)&Req, sizeof(Req), &Resp, sizeof(Resp), &BytesRead, NULL);
        if (!Success) {
            printf("TransactNamedPipe failed, GLE=%d.\n", GetLastError());
            return false;
        }
        if (BytesRead < sizeof(MessageType)) {
            printf("Response too small to even include a message type\n");
            return false;
        }
        if (Resp.Type != Type) {
            printf("Response could not be parsed (returned %d)\n", Resp.Type);
            return false;
        }
        if (BytesRead != sizeof(Resp)) {
            printf("Incompatible response sizes (is %d, should be %zu)\n", BytesRead, sizeof(Resp));
            return false;
        }
        return true;
    }
}