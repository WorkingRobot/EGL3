#include "PlayInfo.h"

#include "../../utils/Align.h"
#include "../../utils/StringCompare.h"
#include "../../utils/Hex.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Storage::Models {
    PlayInfo::PlayInfo(const InstalledGame& Game, Service::Pipe::Client& PipeClient) :
        Game(Game),
        CurrentState(PlayInfoState::Unknown),
        PipeClient(PipeClient),
        PipeContext(nullptr)
    {

    }

    PlayInfo::~PlayInfo()
    {
        PrimaryTask.get();
        Close();
    }

    void PlayInfo::SetState(PlayInfoState NewState)
    {
        OnStateUpdate.emit(CurrentState = NewState);
    }

    void PlayInfo::Begin() {
        PrimaryTask = std::async(std::launch::async, [this]() {
            SetState(PlayInfoState::Opening);
            EGL3_CONDITIONAL_LOG(Game.OpenArchiveRead(), LogLevel::Critical, "Could not open archive");
            EGL3_CONDITIONAL_LOG(PipeClient.OpenArchive(Game.GetPath(), PipeContext), LogLevel::Critical, "Could not open archive");

            SetState(PlayInfoState::Reading);
            EGL3_CONDITIONAL_LOG(PipeClient.ReadArchive(PipeContext), LogLevel::Critical, "Could not read archive");

            SetState(PlayInfoState::Initializing);
            EGL3_CONDITIONAL_LOG(PipeClient.InitializeDisk(PipeContext), LogLevel::Critical, "Could not intialize disk");

            SetState(PlayInfoState::Creating);
            EGL3_CONDITIONAL_LOG(PipeClient.CreateLUT(PipeContext), LogLevel::Critical, "Could not create lut");

            SetState(PlayInfoState::Starting);
            EGL3_CONDITIONAL_LOG(PipeClient.CreateDisk(PipeContext), LogLevel::Critical, "Could not create disk");

            SetState(PlayInfoState::Mounting);
            EGL3_CONDITIONAL_LOG(PipeClient.MountDisk(PipeContext), LogLevel::Critical, "Could not mount disk");

            SetState(PlayInfoState::Playable);
        });
    }

    void PlayInfo::Play(Web::Epic::EpicClientAuthed& Client) {
        PrimaryTask = std::async(std::launch::async, [&, this]() {
            SetState(PlayInfoState::Playing);
            OnPlay(Client);
        });
    }

    bool PlayInfo::IsPlaying() {
        return false;
    }

    void PlayInfo::Close() {
        if (PipeContext) {
            PipeClient.Destruct(PipeContext);
        }
    }

    void PlayInfo::OnPlay(Web::Epic::EpicClientAuthed& Client)
    {
        std::filesystem::path MountPath;
        {
            char DriveLetter = '\0';
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                bool Success = PipeClient.QueryLetter(PipeContext, DriveLetter);
                if (!Success || DriveLetter == '\0') {
                    printf("Could not find mounted drive\n");
                }
            } while (!DriveLetter);
            MountPath = std::string(1, DriveLetter) + ":/";
        }

        std::string ExchangeCode;
        {
            auto Resp = Client.GetExchangeCode();
            EGL3_CONDITIONAL_LOG(!Resp.HasError(), LogLevel::Critical, "Could not get exchange code");
            ExchangeCode = Resp->Code;
        }

        std::stringstream CommandLine;
        CommandLine << Game.GetArchive().GetManifestData()->GetLaunchCommand()
                    << " -AUTH_LOGIN=unused"
                    << " -AUTH_PASSWORD=" << ExchangeCode
                    << " -AUTH_TYPE=exchangecode"
                    << " -epicapp=" << Game.GetArchive().GetHeader()->GetGame()
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

        auto ExePath = MountPath / Game.GetArchive().GetManifestData()->GetLaunchExe();
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
            CloseHandle(ProcInfo.hProcess);
        }
    }
}