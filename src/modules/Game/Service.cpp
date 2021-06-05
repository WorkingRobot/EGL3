#include "Service.h"

#include "../../utils/Log.h"
#include "../../utils/Config.h"
#include "../../utils/Format.h"

#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Modules::Game {
    ServiceModule::ServiceModule(ModuleList& Ctx)
    {
        auto ClientName = Utils::Format("%s/%s", Utils::Config::GetAppName(), Utils::Config::GetAppVersion());

        for (int Idx = 0; Idx < 3 && !Client.IsConnected(); ++Idx) {
            if (Idx != 0) {
                if (PatchService() == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    Client.Connect(ClientName);
                }
            }
            else {
                Client.Connect(ClientName);
            }
        }
        EGL3_VERIFY(Client.IsConnected(), "Could not connect to the service after multiple retries");
    }

    Service::Pipe::Client& ServiceModule::GetClient()
    {
        return Client;
    }

    int ServiceModule::PatchService()
    {
        CHAR FilePath[MAX_PATH];
        if (!GetModuleFileName(NULL, FilePath, MAX_PATH)) {
            printf("Cannot patch service (%d)\n", GetLastError());
            return GetLastError();
        }

        STARTUPINFO StartupInfo{
            .cb = sizeof(STARTUPINFO),
        };

        auto ExePath = std::filesystem::path(FilePath).parent_path() / "EGL3_SRV.exe";
        std::string CommandLineString = Utils::Format("\"%s\" %s", ExePath.string().c_str(), "patch nowait");
        PROCESS_INFORMATION ProcInfo;

        BOOL Ret = CreateProcess(
            NULL,
            CommandLineString.data(),
            NULL, NULL,
            FALSE, // This is true if we were redirecting std outputs (https://github.com/EpicGames/UnrealEngine/blob/2bf1a5b83a7076a0fd275887b373f8ec9e99d431/Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp#L323)
            (DWORD)(NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW),
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