#include "PlayInfo.h"

#include "../game/ArchiveList.h"
#include "../../utils/StringCompare.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Storage::Models {
    PlayInfo::PlayInfo(const InstalledGame& Game) :
        Game(Game),
        CurrentState(PlayInfoState::Reading)
    {
    }

    PlayInfo::~PlayInfo()
    {

    }

    void PlayInfo::SetState(PlayInfoState NewState)
    {
        OnStateUpdate.emit(CurrentState = NewState);
    }

    void PlayInfo::Begin() {
        PrimaryTask = std::async(std::launch::async, [this]() {
            SetState(PlayInfoState::Reading);
            OnRead();

            SetState(PlayInfoState::Initializing);
            OnInitialize();

            SetState(PlayInfoState::Mounting);
            OnMount();

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
        OnClose();
    }

    void PlayInfo::OnRead()
    {
        if (!Game.OpenArchiveRead()) {
            printf("Couldn't open for reading\n");
            return;
        }

        std::vector<MountedFile> MountedFiles;

        const Game::ArchiveList<Game::RunlistId::File> Files(Game.GetArchive());
        int Idx = 0;
        for (auto& File : Files) {
            auto& MntFile = MountedFiles.emplace_back(MountedFile{
                .Path = File.Filename,
                .FileSize = File.FileSize,
                .UserContext = (void*)Idx++
            });
        }

        Disk.emplace(MountedFiles);
        Disk->HandleFileCluster.Set([this](void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) { HandleFileCluster(Ctx, LCN, Buffer); });
    }

    void PlayInfo::OnInitialize()
    {
        Disk->Initialize();
    }

    void PlayInfo::OnMount()
    {
        Disk->Create();
        Disk->Mount();
    }

    void PlayInfo::OnPlay(Web::Epic::EpicClientAuthed& Client)
    {
        std::string ExchangeCode;
        {
            auto Resp = Client.GetExchangeCode();
            EGL3_CONDITIONAL_LOG(!Resp.HasError(), LogLevel::Critical, "Could not get exchange code");
            ExchangeCode = Resp->Code;
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));

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

        uint32_t CreateFlags = NORMAL_PRIORITY_CLASS | DETACHED_PROCESS; // TODO: check if extension is .cmd or .bat
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

        std::string CommandLineString = Utils::Format("\"%s\" %s", Game.GetArchive().GetManifestData()->GetLaunchExe().c_str(), CommandLine.str().c_str());
        printf("%s\n", CommandLineString.c_str());
        PROCESS_INFORMATION ProcInfo;

        BOOL Ret = CreateProcess(
            NULL,
            CommandLineString.data(),
            NULL, NULL,
            FALSE, // This is true if we were redirecting std outputs (https://github.com/EpicGames/UnrealEngine/blob/2bf1a5b83a7076a0fd275887b373f8ec9e99d431/Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp#L323)
            (DWORD)CreateFlags,
            NULL,
            "D:\\", // TODO: don't make this a constant
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

    void PlayInfo::OnClose()
    {
    }

    void PlayInfo::HandleFileCluster(void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) const
    {
        const Game::ArchiveList<Game::RunlistId::File> Files(Game.GetArchive());
        const Game::ArchiveList<Game::RunlistId::ChunkPart> ChunkParts(Game.GetArchive());
        const Game::ArchiveList<Game::RunlistId::ChunkInfo> ChunkInfos(Game.GetArchive());
        const Game::ArchiveList<Game::RunlistId::ChunkData> ChunkDatas(Game.GetArchive());

        auto& File = Files[(int)Ctx];

        uint64_t ByteOffset = LCN * 4096;
        if (File.FileSize < ByteOffset) {
            memset(Buffer, 0, 4096);
            return;
        }

        auto Itr = ChunkParts.begin() + File.ChunkPartDataStartIdx;
        auto EndItr = Itr + File.ChunkPartDataSize;
        for (; Itr != EndItr; ++Itr) {
            if (ByteOffset < Itr->Size) {
                break;
            }
            ByteOffset -= Itr->Size;
        }
        if (Itr == EndItr) {
            memset(Buffer, 0, 4096);
            return;
        }

        uint64_t BytesToRead = 4096;
        while (BytesToRead && Itr != EndItr) {
            auto& Info = ChunkInfos[Itr->ChunkIdx];

            auto BeginChunkDataItr = ChunkDatas.begin() + Info.DataSector * Game::Header::GetSectorSize() + Itr->Offset;
            auto EndChunkDataItr = BeginChunkDataItr + Itr->Size;

            BeginChunkDataItr += ByteOffset;
            auto BeginPtr = &*BeginChunkDataItr;

            uint64_t BytesAvailable = Itr->Size - ByteOffset;

            if (BytesToRead < BytesAvailable) {
                EndChunkDataItr = BeginChunkDataItr + BytesToRead;
                BytesAvailable = BytesToRead;
            }

            std::copy(BeginChunkDataItr, EndChunkDataItr, Buffer + (4096 - BytesToRead));
            BytesToRead -= BytesAvailable;

            ByteOffset = 0;
            Itr++;
        }
    }
}