#include "Server.h"

#include "../../../utils/streams/BufferStream.h"
#include "../../../utils/Log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <aclapi.h>

namespace EGL3::Service::Pipe {
    Server::Server(const char* Name) :
        PipeName(Name),
        ConnectionThread(std::async(std::launch::async, [this]() { HandleConnectionThread(); }))
    {

    }

    Server::~Server()
    {

    }

    void Server::HandleConnectionThread() {
        PSID EveryoneSID = NULL;
        SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        EGL3_VERIFY(AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSID), "Could not get SID");

        SECURITY_ATTRIBUTES SAttr{
            .nLength = sizeof(SECURITY_ATTRIBUTES),
            .lpSecurityDescriptor = NULL,
            .bInheritHandle = FALSE
        };

        EXPLICIT_ACCESS Ace{
            .grfAccessPermissions = FILE_GENERIC_READ | GENERIC_WRITE,
            .grfAccessMode = SET_ACCESS,
            .grfInheritance = NO_INHERITANCE,
            .Trustee = TRUSTEE{
                .TrusteeForm = TRUSTEE_IS_SID,
                .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                .ptstrName = (LPTSTR)EveryoneSID
            }
        };

        PACL Acl = NULL;
        EGL3_VERIFY(SetEntriesInAcl(1, &Ace, NULL, &Acl) == ERROR_SUCCESS, "Could not set ACL entries");
        auto Sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        EGL3_VERIFY(InitializeSecurityDescriptor(Sd, SECURITY_DESCRIPTOR_REVISION), "Could not allocate SD");
        EGL3_VERIFY(SetSecurityDescriptorDacl(Sd, TRUE, Acl, FALSE), "Could not set dacl");

        SAttr.lpSecurityDescriptor = Sd;

        while (true) {
            HANDLE PipeHandle = CreateNamedPipe(PipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, PIPE_UNLIMITED_INSTANCES, BufferSize, BufferSize, 0, &SAttr);
            if (PipeHandle == INVALID_HANDLE_VALUE) {
                printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
                break;
            }

            if (ConnectNamedPipe(PipeHandle, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                ConnectedThreads.emplace_back(std::async(std::launch::async, [this, PipeHandle]() { HandleConnection(PipeHandle); }));
            }
            else {
                CloseHandle(PipeHandle);
            }
        }

        LocalFree(Sd);
        LocalFree(Acl);
        FreeSid(EveryoneSID);
    }

    thread_local std::vector<std::shared_ptr<MountedArchive>> ConnectionMounts;

    void Server::HandleConnection(void* PipeHandle)
    {
        if (PipeHandle == NULL) {
            return;
        }

        char RequestBuffer[BufferSize];
        char ResponseBuffer[BufferSize];

        while (true) {
            DWORD BytesRead;
            
            bool Success = ReadFile(PipeHandle, RequestBuffer, BufferSize, &BytesRead, NULL);

            if (!Success || BytesRead == 0) {
                DWORD Error = GetLastError();
                if (Error == ERROR_BROKEN_PIPE) {
                    printf("Client disconnected.\n");
                }
                else {
                    printf("ReadFile failed, GLE=%d.\n", Error);
                }
                break;
            }

            uint32_t BytesToWrite;
            HandlePacket(RequestBuffer, BytesRead, ResponseBuffer, BytesToWrite);

            DWORD BytesWritten;
            Success = WriteFile(PipeHandle, ResponseBuffer, BytesToWrite, &BytesWritten, NULL);

            if (!Success || BytesToWrite != BytesWritten) {
                printf("WriteFile failed, GLE=%d\n", GetLastError());
                break;
            }
        }

        FlushFileBuffers(PipeHandle);
        DisconnectNamedPipe(PipeHandle);
        CloseHandle(PipeHandle);

        ConnectionMounts.clear();
    }

    template<>
    void Server::HandlePacket<PacketId::Handshake>(const Packet<PacketId::Handshake, PacketDirection::ServerBound>& Input, Packet<PacketId::Handshake, PacketDirection::ClientBound>& Output)
    {
        if (Input.Protocol > PacketProtocol::Latest) {
            Output.Response = PacketResponse::ServerOutdated;
        }
        else if (Input.Protocol < PacketProtocol::Latest) {
            Output.Response = PacketResponse::ClientOutdated;
        }
        else {
            Output.Response = PacketResponse::Success;
        }
        Output.ServerName = SERVICE_NAME "/" CONFIG_VERSION_LONG;
    }

    template<>
    void Server::HandlePacket<PacketId::BeginMount>(const Packet<PacketId::BeginMount, PacketDirection::ServerBound>& Input, Packet<PacketId::BeginMount, PacketDirection::ClientBound>& Output)
    {
        {
            uint64_t Idx = 1;
            std::shared_lock Lock(MountsMtx);
            for (auto& Mount : Mounts) {
                auto Ptr = Mount.lock();
                if (Ptr) {
                    if (Ptr->GetArchivePath() == Input.Path) {
                        Output.Response = PacketResponse::ArchiveAlreadyExists;
                        Output.MountHandle = MountHandle(Idx);
                        ConnectionMounts.emplace_back(std::move(Ptr));
                        return;
                    }
                }
                ++Idx;
            }
        }

        Storage::Game::Archive Archive(Input.Path, Storage::Game::ArchiveMode::Read);
        if (!Archive.IsValid()) {
            Output.Response = PacketResponse::InvalidArchive;
            Output.MountHandle = 0;
            return;
        }

        std::lock_guard Guard(MountsMtx);
        Output.Response = PacketResponse::Success;
        Output.MountHandle = MountHandle(Mounts.size() + 1);
        auto& Ptr = ConnectionMounts.emplace_back(std::make_shared<MountedArchive>(Input.Path, std::move(Archive)));
        Mounts.emplace_back(Ptr);
    }

    template<>
    void Server::HandlePacket<PacketId::GetMountPath>(const Packet<PacketId::GetMountPath, PacketDirection::ServerBound>& Input, Packet<PacketId::GetMountPath, PacketDirection::ClientBound>& Output)
    {
        auto Ptr = GetMount(Input.MountHandle);
        if (!Ptr) {
            Output.Response = PacketResponse::InvalidMountHandle;
            Output.Path.clear();
            return;
        }
        char Letter = Ptr->GetDriveLetter();
        if (!Letter) {
            Output.Response = PacketResponse::NoMountPathForArchiveYet;
            Output.Path.clear();
            return;
        }
        Output.Response = PacketResponse::Success;
        Output.Path = std::string(1, Letter) + ":\\";
    }

    template<>
    void Server::HandlePacket<PacketId::GetMountInfo>(const Packet<PacketId::GetMountInfo, PacketDirection::ServerBound>& Input, Packet<PacketId::GetMountInfo, PacketDirection::ClientBound>& Output)
    {
        auto Ptr = GetMount(Input.MountHandle);
        if (!Ptr) {
            Output.Response = PacketResponse::InvalidMountHandle;
            return;
        }
        Output.Response = PacketResponse::Success;
    }

    template<>
    void Server::HandlePacket<PacketId::EndMount>(const Packet<PacketId::EndMount, PacketDirection::ServerBound>& Input, Packet<PacketId::EndMount, PacketDirection::ClientBound>& Output)
    {
        auto PtrWeak = GetMountWeak(Input.MountHandle);
        {
            auto Ptr = PtrWeak.lock();
            if (!Ptr) {
                Output.Response = PacketResponse::InvalidMountHandle;
                return;
            }
            std::remove(ConnectionMounts.begin(), ConnectionMounts.end(), Ptr);
        }
        Output.Response = PtrWeak.expired() ? PacketResponse::Success : PacketResponse::ArchiveStillExists;
    }

    template<PacketId Id>
    void Server::HandlePacket(const char Input[BufferSize], PacketHeader InputHeader, char Output[BufferSize], uint32_t& OutputSize)
    {
        Packet<Id, PacketDirection::ServerBound> InputPacket;
        Packet<Id, PacketDirection::ClientBound> OutputPacket;
        {
            Utils::Streams::BufferStream Stream((char*)Input + sizeof(PacketHeader), InputHeader.GetSize());
            Stream >> InputPacket;
        }
        HandlePacket(InputPacket, OutputPacket);
        PacketHeader OutputHeader(Id, 0);
        {
            Utils::Streams::BufferStream Stream(Output + sizeof(PacketHeader), BufferSize - sizeof(PacketHeader));
            Stream << OutputPacket;
            if (sizeof(PacketHeader) + Stream.tell() > BufferSize) {
                // Output packet is too large
                OutputSize = 0;
                return;
            }
            OutputSize = sizeof(PacketHeader) + Stream.tell();
            OutputHeader.SetSize(Stream.tell());
        }
        *((PacketHeader*)Output) = OutputHeader;
    }

    void Server::HandlePacket(const char Input[BufferSize], uint32_t InputSize, char Output[BufferSize], uint32_t& OutputSize)
    {
        if (InputSize < sizeof(PacketHeader)) {
            // Input is too small to even get a header
            OutputSize = 0;
            return;
        }

        PacketHeader RequestHeader = *(PacketHeader*)Input;
        if (InputSize != RequestHeader.GetSize() + sizeof(PacketHeader)) {
            // Request's packet header does not match the number of input bytes
            OutputSize = 0;
            return;
        }
        switch (RequestHeader.GetId())
        {
        case PacketId::Handshake:
            HandlePacket<PacketId::Handshake>(Input, RequestHeader, Output, OutputSize);
            break;
        case PacketId::BeginMount:
            HandlePacket<PacketId::BeginMount>(Input, RequestHeader, Output, OutputSize);
            break;
        case PacketId::GetMountPath:
            HandlePacket<PacketId::GetMountPath>(Input, RequestHeader, Output, OutputSize);
            break;
        case PacketId::GetMountInfo:
            HandlePacket<PacketId::GetMountInfo>(Input, RequestHeader, Output, OutputSize);
            break;
        case PacketId::EndMount:
            HandlePacket<PacketId::EndMount>(Input, RequestHeader, Output, OutputSize);
            break;
        default:
            // Request has invalid packet id
            OutputSize = 0;
            break;
        }
    }

    std::weak_ptr<MountedArchive> Server::GetMountWeak(MountHandle MountHandle)
    {
        std::shared_lock Lock(MountsMtx);
        if (uint64_t(MountHandle) - 1 >= Mounts.size()) {
            return {};
        }
        return Mounts[uint64_t(MountHandle) - 1];
    }

    std::shared_ptr<MountedArchive> Server::GetMount(MountHandle MountHandle)
    {
        return GetMountWeak(MountHandle).lock();
    }
}