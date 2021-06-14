#include "Client.h"

#include "../../../utils/streams/BufferStream.h"
#include "../../../utils/Log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Service::Pipe {
    Client::Client() :
        PipeHandle(NULL),
        Connected(false)
    {

    }

    Client::~Client()
    {
        CloseHandle(PipeHandle);
    }

    bool Client::Connect(const std::string& ClientName, const char* Name)
    {
        CloseHandle(PipeHandle);
        Connected = false;

        while (true) {
            PipeHandle = CreateFile(Name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

            if (PipeHandle != INVALID_HANDLE_VALUE) {
                break;
            }

            if (!EGL3_ENSUREF(GetLastError() == ERROR_PIPE_BUSY, LogLevel::Error, "CreateFile failed (GLE: {})", GetLastError())) {
                return false;
            }

            if (!EGL3_ENSUREF(WaitNamedPipe(Name, 1000), LogLevel::Error, "WaitNamedPipe timed out (GLE: {})", GetLastError())) {
                return false;
            }
        }

        DWORD PipeMode = PIPE_READMODE_MESSAGE;
        bool Success = SetNamedPipeHandleState(PipeHandle, &PipeMode, NULL, NULL);
        if (!EGL3_ENSUREF(Success, LogLevel::Error, "SetNamedPipeHandleState failed (GLE: {})", GetLastError())) {
            return false;
        }

        auto Resp = Handshake(PacketProtocol::Latest, ClientName, ServerName);
        if (!EGL3_ENSUREF(Resp == PacketResponse::Success, LogLevel::Error, "Handshake failed (GLE: {}, Response: {})", GetLastError(), (uint16_t)Resp)) {
            return false;
        }

        Connected = true;
        return true;
    }

    bool Client::IsConnected() const
    {
        return Connected;
    }

    const std::string& Client::GetServerName() const
    {
        return ServerName;
    }

    PacketResponse Client::BeginMount(const std::filesystem::path& Path, MountHandle& Handle)
    {
        Packet<PacketId::BeginMount, PacketDirection::ClientBound> Resp;

        bool Success = Transact(Packet<PacketId::BeginMount, PacketDirection::ServerBound> {
            .Path = Path
        }, Resp);

        Handle = Resp.MountHandle;

        return Success ? Resp.Response : PacketResponse::TransactionFailure;
    }

    PacketResponse Client::GetMountPath(MountHandle Handle, std::filesystem::path& Path)
    {
        Packet<PacketId::GetMountPath, PacketDirection::ClientBound> Resp;

        bool Success = Transact(Packet<PacketId::GetMountPath, PacketDirection::ServerBound> {
            .MountHandle = Handle
        }, Resp);

        Path = Resp.Path;

        return Success ? Resp.Response : PacketResponse::TransactionFailure;
    }

    PacketResponse Client::GetMountInfo(MountHandle Handle)
    {
        Packet<PacketId::GetMountInfo, PacketDirection::ClientBound> Resp;

        bool Success = Transact(Packet<PacketId::GetMountInfo, PacketDirection::ServerBound> {
            .MountHandle = Handle
        }, Resp);

        return Success ? Resp.Response : PacketResponse::TransactionFailure;
    }

    PacketResponse Client::EndMount(MountHandle Handle)
    {
        Packet<PacketId::EndMount, PacketDirection::ClientBound> Resp;

        bool Success = Transact(Packet<PacketId::EndMount, PacketDirection::ServerBound> {
            .MountHandle = Handle
        }, Resp);

        return Success ? Resp.Response : PacketResponse::TransactionFailure;
    }

    PacketResponse Client::Handshake(PacketProtocol Protocol, const std::string& ClientName, std::string& ServerName)
    {
        Packet<PacketId::Handshake, PacketDirection::ClientBound> Resp;

        bool Success = Transact(Packet<PacketId::Handshake, PacketDirection::ServerBound> {
            .Protocol = Protocol,
            .ClientName = ClientName
        }, Resp);

        ServerName = Resp.ServerName;

        return Success ? Resp.Response : PacketResponse::TransactionFailure;
    }

    template<PacketId Id>
    bool Client::Transact(const Packet<Id, PacketDirection::ServerBound>& Request, Packet<Id, PacketDirection::ClientBound>& Response)
    {
        char RequestBuffer[BufferSize];
        PacketHeader RequestHeader(Id, 0);
        {
            Utils::Streams::BufferStream Stream(RequestBuffer + sizeof(PacketHeader), BufferSize - sizeof(PacketHeader));
            Stream << Request;
            if (!EGL3_ENSUREF(Stream.tell() + sizeof(PacketHeader) <= BufferSize, LogLevel::Error, "Request is too large ({})", Stream.tell() + sizeof(PacketHeader))) {
                return false;
            }
            RequestHeader.SetSize(Stream.tell());
        }
        *((PacketHeader*)RequestBuffer) = RequestHeader;

        char ResponseBuffer[BufferSize];
        DWORD BytesRead = 0;
        bool Success = TransactNamedPipe(PipeHandle, RequestBuffer, RequestHeader.GetSize() + sizeof(PacketHeader), ResponseBuffer, sizeof(ResponseBuffer), &BytesRead, NULL);
        if (!EGL3_ENSUREF(Success, LogLevel::Error, "TransactNamedPipe failed (GLE: {})", GetLastError())) {
            return false;
        }
        if (!EGL3_ENSURE(BytesRead >= sizeof(PacketHeader), LogLevel::Error, "Response too small to even include a message type")) {
            return false;
        }
        PacketHeader ResponseHeader = *(PacketHeader*)ResponseBuffer;
        if (!EGL3_ENSURE(BytesRead == ResponseHeader.GetSize() + sizeof(PacketHeader), LogLevel::Error, "Response's packet header does not match the number of bytes read")) {
            return false;
        }
        if (!EGL3_ENSUREF(Id == ResponseHeader.GetId(), LogLevel::Error, "Response returned the wrong packet type ({})", (uint16_t)ResponseHeader.GetId())) {
            return false;
        }
        {
            Utils::Streams::BufferStream Stream(ResponseBuffer + sizeof(PacketHeader), ResponseHeader.GetSize());
            Stream >> Response;
        }
        return true;
    }
}