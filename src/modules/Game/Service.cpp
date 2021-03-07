#include "Service.h"

#include "../../utils/Format.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Modules::Game {
    ServiceModule::ServiceModule()
    {
        Client.emplace();

        if (!EGL3_CONDITIONAL_LOG(Client->IsConnected(), LogLevel::Error, "Could not connect to service server")) {
            for (int i = 0; i < 5 && PatchService(); ++i) {} // 5 retries

            // Assignment operator without the optional goes bonkers, this is a good alternative
            Client.reset();
            Client.emplace();
            EGL3_CONDITIONAL_LOG(Client->IsConnected(), LogLevel::Critical, "Could not connect to service server (after relaunch)");
        }

        uint32_t ProtocolVersion, MountedDiskCount;
        EGL3_CONDITIONAL_LOG(Client->QueryServer(ProtocolVersion, MountedDiskCount), LogLevel::Critical, "Could not query service server");
        EGL3_CONDITIONAL_LOG(ProtocolVersion == Service::Pipe::ProtocolVersion, LogLevel::Critical, "Service server protocol version does not match");
    }

    Service::Pipe::Client& ServiceModule::GetClient()
    {
        return *Client;
    }

    int ServiceModule::PatchService()
    {
        CHAR FilePath[MAX_PATH];
        if (!GetModuleFileName(NULL, FilePath, MAX_PATH)) {
            printf("Cannot patch service (%d)\n", GetLastError());
            return GetLastError();
        }

        uint32_t CreateFlags = NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW;
        uint32_t DwFlags = 0;
        uint16_t ShowWindowFlags = SW_HIDE;

        STARTUPINFO StartupInfo{
            .cb = sizeof(STARTUPINFO),

            .lpReserved = NULL,
            .lpDesktop = NULL,
            .lpTitle = NULL,

            .dwX = (DWORD)CW_USEDEFAULT,
            .dwY = (DWORD)CW_USEDEFAULT,
            .dwXSize = (DWORD)CW_USEDEFAULT,
            .dwYSize = (DWORD)CW_USEDEFAULT,
            .dwXCountChars = (DWORD)0,
            .dwYCountChars = (DWORD)0,
            .dwFillAttribute = (DWORD)0,
            .dwFlags = (DWORD)DwFlags,
            .wShowWindow = (WORD)ShowWindowFlags,

            .cbReserved2 = 0,
            .lpReserved2 = NULL,
            .hStdInput = HANDLE(NULL),
            .hStdOutput = HANDLE(NULL),
            .hStdError = HANDLE(NULL)
        };

        auto ExePath = std::filesystem::path(FilePath).parent_path() / "EGL3_SRV.exe";
        std::string CommandLineString = Utils::Format("\"%s\" %s", ExePath.string().c_str(), "patch nowait");
        PROCESS_INFORMATION ProcInfo;

        BOOL Ret = CreateProcess(
            NULL,
            CommandLineString.data(),
            NULL, NULL,
            FALSE, // This is true if we were redirecting std outputs (https://github.com/EpicGames/UnrealEngine/blob/2bf1a5b83a7076a0fd275887b373f8ec9e99d431/Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp#L323)
            (DWORD)CreateFlags,
            NULL,
            ExePath.parent_path().string().c_str(),
            &StartupInfo,
            &ProcInfo
        );
        if (!Ret) {
            return GetLastError();
        }
        else {
            CloseHandle(ProcInfo.hThread);

            WaitForSingleObject(ProcInfo.hProcess, INFINITE);

            DWORD ReturnCode;
            if (!GetExitCodeProcess(ProcInfo.hProcess, &ReturnCode)) {
                ReturnCode = GetLastError();
            }

            CloseHandle(ProcInfo.hProcess);

            return ReturnCode;
        }
    }
}