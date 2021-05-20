#include "PlayInfo.h"

#include "../../utils/Align.h"
#include "../../utils/StringCompare.h"
#include "../../utils/Hex.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Storage::Models {
    PlayInfo::PlayInfo(InstalledGame& Game) :
        Game(Game),
        CurrentState(PlayInfoState::Constructed),
        ProcessHandle(nullptr)
    {

    }

    PlayInfo::~PlayInfo()
    {
        if (PrimaryTask.valid()) {
            PrimaryTask.get();
        }
        Game.Unmount();
        if (ProcessHandle) {
            CloseHandle(ProcessHandle);
        }
    }

    PlayInfoState PlayInfo::GetState() const
    {
        return CurrentState;
    }

    void PlayInfo::SetState(PlayInfoState NewState)
    {
        OnStateUpdate.emit(CurrentState = NewState);
    }

    void PlayInfo::Mount(Service::Pipe::Client& PipeClient)
    {
        if (CurrentState != PlayInfoState::Constructed) {
            return;
        }
        
        PrimaryTask = std::async(std::launch::async, [this, Client = std::reference_wrapper<Service::Pipe::Client>(PipeClient)]() {
            SetState(PlayInfoState::Mounting);
            Game.Mount(Client);
            SetState(PlayInfoState::Playable);
        });
    }

    void PlayInfo::Play(Web::Epic::EpicClientAuthed& Client) {
        if (CurrentState != PlayInfoState::Playable) {
            return;
        }

        PrimaryTask = std::async(std::launch::async, [&, this]() {
            SetState(PlayInfoState::Playing);
            OnPlay(Client);
            if (ProcessHandle) {
                WaitForSingleObject(ProcessHandle, INFINITE);
            }
            SetState(PlayInfoState::Playable);
        });
    }

    void PlayInfo::OnPlay(Web::Epic::EpicClientAuthed& Client)
    {
        // TODO: make this a retry system?
        while (Game.GetMountPath().empty()) {
            printf("Searching for mounted drive\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        std::string ExchangeCode;
        {
            auto Resp = Client.GetExchangeCode();
            EGL3_CONDITIONAL_LOG(!Resp.HasError(), LogLevel::Critical, "Could not get exchange code");
            ExchangeCode = Resp->Code;
        }

        std::stringstream CommandLine;
        CommandLine << Game.GetManifestData()->GetLaunchCommand()
                    << " -AUTH_LOGIN=unused"
                    << " -AUTH_PASSWORD=" << ExchangeCode
                    << " -AUTH_TYPE=exchangecode"
                    << " -epicapp=" << Game.GetHeader()->GetGame()
                    << " -epicenv=" << "Prod"
                    // no ownership token support just yet, Fortnite is free to play after all
                    << " -EpicPortal"
                    << " -epicusername=\"" << Client.GetAuthData().DisplayName.value() << "\""
                    << " -epicuserid=" << Client.GetAuthData().AccountId.value()
                    << " -epiclocale=en";

        uint32_t CreateFlags = NORMAL_PRIORITY_CLASS | DETACHED_PROCESS;
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

        auto ExePath = Game.GetMountPath() / Game.GetManifestData()->GetLaunchExe();
        std::string CommandLineString = Utils::Format("\"%s\" %s", ExePath.string().c_str(), CommandLine.str().c_str());
        printf("%s\n", CommandLineString.c_str());
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
            DWORD ErrorCode = GetLastError();
            printf("%u\n", ErrorCode);
        }
        else {
            CloseHandle(ProcInfo.hThread);
            ProcessHandle = ProcInfo.hProcess;
        }
    }
}