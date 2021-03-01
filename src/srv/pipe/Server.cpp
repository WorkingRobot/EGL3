#include "Server.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <aclapi.h>

namespace EGL3::Service::Pipe {
    Server::Server(const char* Name) :
        PipeName(Name)
    {
        ConnectionThread = std::async(std::launch::async, [this]() { HandleConnectionThread(); });
    }

    Server::~Server()
    {

    }

    void Server::HandleConnectionThread() {
        PSID EveryoneSID = NULL;
        SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        EGL3_CONDITIONAL_LOG(AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSID), LogLevel::Critical, "Could not get SID");

        SECURITY_ATTRIBUTES SAttr{
            .nLength = sizeof(SECURITY_ATTRIBUTES),
            .lpSecurityDescriptor = NULL,
            .bInheritHandle = FALSE
        };

        EXPLICIT_ACCESS Ace{
            .grfAccessPermissions = FILE_GENERIC_READ | GENERIC_WRITE ,
            .grfAccessMode = SET_ACCESS,
            .grfInheritance = NO_INHERITANCE,
            .Trustee = TRUSTEE{
                .TrusteeForm = TRUSTEE_IS_SID,
                .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                .ptstrName = (LPTSTR)EveryoneSID
            }
        };

        PACL Acl = NULL;
        EGL3_CONDITIONAL_LOG(SetEntriesInAcl(1, &Ace, NULL, &Acl) == ERROR_SUCCESS, LogLevel::Critical, "Could not set ACL entries");
        auto Sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        EGL3_CONDITIONAL_LOG(InitializeSecurityDescriptor(Sd, SECURITY_DESCRIPTOR_REVISION), LogLevel::Critical, "Could not allocate SD");
        EGL3_CONDITIONAL_LOG(SetSecurityDescriptorDacl(Sd, TRUE, Acl, FALSE), LogLevel::Critical, "Could not set dacl");

        SAttr.lpSecurityDescriptor = Sd;

        while (true) {
            HANDLE PipeHandle = CreateNamedPipe(PipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BufferSize, BufferSize, 0, &SAttr);
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

    void Server::HandleConnection(void* PipeHandle)
    {
        if (PipeHandle == NULL) {
            return;
        }

        auto BufRequest = std::make_unique<char[]>(BufferSize);
        auto BufResponse = std::make_unique<char[]>(BufferSize);

        while (true) {
            DWORD BytesRead;

            bool Success = ReadFile(PipeHandle, BufRequest.get(), BufferSize, &BytesRead, NULL);

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
            HandleRequest(BufRequest, BytesRead, BufResponse, BytesToWrite);

            DWORD BytesWritten;
            Success = WriteFile(PipeHandle, BufResponse.get(), BytesToWrite, &BytesWritten, NULL);

            if (!Success || BytesToWrite != BytesWritten) {
                printf("WriteFile failed, GLE=%d\n", GetLastError());
                break;
            }
        }

        FlushFileBuffers(PipeHandle);
        DisconnectNamedPipe(PipeHandle);
        CloseHandle(PipeHandle);
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::OpenArchive>(const Request<MessageType::OpenArchive>& Input, Response<MessageType::OpenArchive>& Output)
    {
        Output.Context = NULL;

        if (wcsnlen_s(Input.FilePath, _countof(Input.FilePath)) == _countof(Input.FilePath)) {
            Output.Status = ResponseStatus::InvalidPath;
            return;
        }

        std::filesystem::path Path(Input.FilePath);
        if (Path.empty()) {
            Output.Status = ResponseStatus::InvalidPath;
            return;
        }

        auto Archive = std::make_unique<MountedArchive>(Path);
        if (!Archive->OpenArchive()) {
            Output.Status = ResponseStatus::Failure;
        }
        else {
            Output.Context = Mounts.emplace_back(std::move(Archive)).get();
            Output.Status = ResponseStatus::Success;
        }
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::ReadArchive>(const Request<MessageType::ReadArchive>& Input, Response<MessageType::ReadArchive>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        Output.Status = Data->ReadArchive() ? ResponseStatus::Success : ResponseStatus::Failure;
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::InitializeDisk>(const Request<MessageType::InitializeDisk>& Input, Response<MessageType::InitializeDisk>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        Output.Status = Data->InitializeDisk() ? ResponseStatus::Success : ResponseStatus::Failure;
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::CreateLUT>(const Request<MessageType::CreateLUT>& Input, Response<MessageType::CreateLUT>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        Output.Status = Data->CreateLUT() ? ResponseStatus::Success : ResponseStatus::Failure;
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::CreateDisk>(const Request<MessageType::CreateDisk>& Input, Response<MessageType::CreateDisk>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        Output.Status = Data->CreateDisk() ? ResponseStatus::Success : ResponseStatus::Failure;
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::MountDisk>(const Request<MessageType::MountDisk>& Input, Response<MessageType::MountDisk>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        Output.Status = Data->MountDisk() ? ResponseStatus::Success : ResponseStatus::Failure;
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::Destruct>(const Request<MessageType::Destruct>& Input, Response<MessageType::Destruct>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        auto Itr = std::find_if(Mounts.begin(), Mounts.end(), [&Data](const std::unique_ptr<MountedArchive>& Ptr) { return Ptr.get() == Data; });
        if (Itr != Mounts.end()) {
            Mounts.erase(Itr);
            Output.Status = ResponseStatus::Success;
        }
        else {
            Output.Status = ResponseStatus::Failure;
        }
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::QueryPath>(const Request<MessageType::QueryPath>& Input, Response<MessageType::QueryPath>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        wcscpy_s(Output.FilePath, Data->QueryPath().c_str());
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::QueryLetter>(const Request<MessageType::QueryLetter>& Input, Response<MessageType::QueryLetter>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;
        {
            HANDLE hEventSource;
            LPCSTR lpszStrings[2];
            CHAR Buffer[80];

            hEventSource = RegisterEventSource(NULL, SERVICE_NAME);

            if (NULL != hEventSource)
            {
                sprintf_s(Buffer, "querying letter");

                lpszStrings[0] = SERVICE_NAME;
                lpszStrings[1] = Buffer;

                ReportEvent(hEventSource,// event log handle
                    EVENTLOG_ERROR_TYPE, // event type
                    0,                   // event category
                    ((DWORD)0xC0020001L),           // event identifier
                    NULL,                // no security identifier
                    2,                   // size of lpszStrings array
                    0,                   // no binary data
                    lpszStrings,         // array of strings
                    NULL);               // no binary data

                DeregisterEventSource(hEventSource);
            }
        }
        Output.Letter = Data->QueryDriveLetter();
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::QueryStats>(const Request<MessageType::QueryStats>& Input, Response<MessageType::QueryStats>& Output)
    {
        auto Data = (MountedArchive*)Input.Context;

        Output.Stats = Data->QueryStats();
    }

    template<>
    void Pipe::Server::HandleRequest<MessageType::QueryServer>(const Request<MessageType::QueryServer>& Input, Response<MessageType::QueryServer>& Output)
    {
        Output.ProtocolVersion = ProtocolVersion;
        Output.MountedDiskCount = Mounts.size();
    }

    template<MessageType Type>
    void Server::HandleRequestTyped(const std::unique_ptr<char[]>& Input, uint32_t InputSize, std::unique_ptr<char[]>& Output, uint32_t& OutputSize)
    {
        std::construct_at((Response<Type>*)Output.get());

        HandleRequest<Type>(*(Request<Type>*)Input.get(), *(Response<Type>*)Output.get());

        OutputSize = sizeof(Response<Type>);
    }

    void Server::HandleRequest(const std::unique_ptr<char[]>& Input, uint32_t InputSize, std::unique_ptr<char[]>& Output, uint32_t& OutputSize)
    {
        if (InputSize < sizeof(MessageType)) {
            *(MessageType*)Output.get() = MessageType::Error;
            OutputSize = sizeof(MessageType);
            return;
        }

        switch (*(MessageType*)Input.get())
        {
        case MessageType::OpenArchive:
            HandleRequestTyped<MessageType::OpenArchive>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::ReadArchive:
            HandleRequestTyped<MessageType::ReadArchive>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::InitializeDisk:
            HandleRequestTyped<MessageType::InitializeDisk>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::CreateLUT:
            HandleRequestTyped<MessageType::CreateLUT>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::CreateDisk:
            HandleRequestTyped<MessageType::CreateDisk>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::MountDisk:
            HandleRequestTyped<MessageType::MountDisk>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::Destruct:
            HandleRequestTyped<MessageType::Destruct>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::QueryPath:
            HandleRequestTyped<MessageType::QueryPath>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::QueryLetter:
            HandleRequestTyped<MessageType::QueryLetter>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::QueryStats:
            HandleRequestTyped<MessageType::QueryStats>(Input, InputSize, Output, OutputSize);
            break;
        case MessageType::QueryServer:
            HandleRequestTyped<MessageType::QueryServer>(Input, InputSize, Output, OutputSize);
            break;
        default:
            std::construct_at((Response<MessageType::Error>*)Output.get());
            OutputSize = sizeof(Response<MessageType::Error>);
            break;
        }
    }
}