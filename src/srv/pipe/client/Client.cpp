#include "Client.h"

#include "../../../utils/streams/BufferStream.h"

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

            if (GetLastError() != ERROR_PIPE_BUSY) {
                printf("CreateFile failed, GLE=%d.\n", GetLastError());
                return false;
            }

            if (!WaitNamedPipe(Name, 1000)) {
                printf("WaitNamedPipe timed out, GLE=%d.\n", GetLastError());
                return false;
            }
        }

        DWORD PipeMode = PIPE_READMODE_MESSAGE;
        bool Success = SetNamedPipeHandleState(PipeHandle, &PipeMode, NULL, NULL);
        if (!Success) {
            printf("SetNamedPipeHandleState failed, GLE=%d.\n", GetLastError());
            return false;
        }

        auto Resp = Handshake(PacketProtocol::Latest, ClientName, ServerName);
        if (Resp != PacketResponse::Success) {
            printf("Handshake failed, GLE=%d, Response=%hu.\n", GetLastError(), Resp);
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
            if (Stream.tell() + sizeof(PacketHeader) > BufferSize) {
                printf("Request is too large (%zu)\n", Stream.tell() + sizeof(PacketHeader));
                return false;
            }
            RequestHeader.SetSize(Stream.tell());
        }
        *((PacketHeader*)RequestBuffer) = RequestHeader;

        char ResponseBuffer[BufferSize];
        DWORD BytesRead = 0;
        bool Success = TransactNamedPipe(PipeHandle, RequestBuffer, RequestHeader.GetSize() + sizeof(PacketHeader), ResponseBuffer, sizeof(ResponseBuffer), &BytesRead, NULL);
        if (!Success) {
            printf("TransactNamedPipe failed, GLE=%d.\n", GetLastError());
            return false;
        }
        if (BytesRead < sizeof(PacketHeader)) {
            printf("Response too small to even include a message type\n");
            return false;
        }
        PacketHeader ResponseHeader = *(PacketHeader*)ResponseBuffer;
        if (BytesRead != ResponseHeader.GetSize() + sizeof(PacketHeader)) {
            printf("Response's packet header does not match the number of bytes read\n");
            return false;
        }
        if (Id != ResponseHeader.GetId()) {
            printf("Response returned the wrong packet type (returned %hu)\n", ResponseHeader.GetId());
            return false;
        }
        {
            Utils::Streams::BufferStream Stream(ResponseBuffer + sizeof(PacketHeader), ResponseHeader.GetSize());
            Stream >> Response;
        }
        return true;
    }
}