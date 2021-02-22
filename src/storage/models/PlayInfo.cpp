#include "PlayInfo.h"

#include "../../utils/Align.h"
#include "../../utils/StringCompare.h"
#include "../../utils/Hex.h"

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

        Disk.emplace(MountedFiles, Utils::Random());
        Disk->HandleFileCluster.Set([this](void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) { HandleFileCluster(Ctx, LCN, Buffer); });
    }

    void PlayInfo::OnInitialize()
    {
        Disk->Initialize();

        {
            ArchiveLists.emplace(Game.GetArchive());

            SectionLUT.reserve(ArchiveLists->Files.size());
            for (auto& File : ArchiveLists->Files) {
                //printf("%s %s\n", File.Filename, Utils::ToHex<true>(File.SHA).c_str());
                auto& Sections = SectionLUT.emplace_back();
                uint32_t ClusterCount = Utils::Align<4096>(File.FileSize) / 4096;
                Sections.reserve(ClusterCount);

                auto GetDataItr = [&](uint32_t ChunkIdx) -> uint64_t {
                    auto& Info = ArchiveLists->ChunkInfos[ChunkIdx];
                    return Info.DataSector * Game::Header::GetSectorSize();
                };

                auto Itr = ArchiveLists->ChunkParts.begin() + File.ChunkPartDataStartIdx;
                auto EndItr = Itr + File.ChunkPartDataSize;
                uint32_t ItrDataOffset = 0;
                for (uint32_t i = 0; i < ClusterCount; ++i) {
                    uint32_t BytesToSelect = std::min(4096llu, File.FileSize - i * 4096llu);
                    auto& Cluster = Sections.emplace_back();
                    int j = 0;

                    do {
                        EGL3_CONDITIONAL_LOG(j < 4, LogLevel::Critical, "Not enough available sections for this cluster");
                        EGL3_CONDITIONAL_LOG(ItrDataOffset <= Itr->Size, LogLevel::Critical, "Invalid data offset");
                        if (ItrDataOffset == Itr->Size) {
                            ++Itr;
                            ItrDataOffset = 0;
                        }
                        auto& ChunkPart = *Itr;
                        auto& Section = Cluster.Sections[j];
                        Section.Ptr = GetDataItr(ChunkPart.ChunkIdx) + ChunkPart.Offset + ItrDataOffset;
                        Section.Size = std::min(ChunkPart.Size - ItrDataOffset, BytesToSelect);
                        BytesToSelect -= Section.Size;
                        ItrDataOffset += Section.Size;
                        ++j;
                    } while (BytesToSelect);

                    EGL3_CONDITIONAL_LOG(Cluster.TotalSize() == std::min(4096llu, File.FileSize - i * 4096llu), LogLevel::Critical, "Invalid cluster size");
                }
            }
        }
    }

    void PlayInfo::OnMount()
    {
        Disk->Create();
        Disk->Mount();
    }

    void PlayInfo::OnPlay(Web::Epic::EpicClientAuthed& Client)
    {
        std::filesystem::path MountPath;
        {
            char DriveLetter = '\0';
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                DriveLetter = Disk->GetDriveLetter();
                if (DriveLetter == '\0') {
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

    void PlayInfo::OnClose()
    {
    }

    template<class> inline constexpr bool always_false_v = false;

    // Technically VCN, but we aren't even using NTFS anymore
    void PlayInfo::HandleFileCluster(void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) const
    {
        auto& File = SectionLUT[(size_t)Ctx];

        if (File.size() < LCN) {
            memset(Buffer, 0, 4096);
            return;
        }

        SectionPart* Itr = (SectionPart*)&File[LCN].Sections;
        while (Itr->Size) {
            std::copy(ArchiveLists->ChunkDatas.begin() + Itr->Ptr, ArchiveLists->ChunkDatas.begin() + Itr->Ptr + Itr->Size, Buffer);
            Buffer = (uint8_t*)Buffer + Itr->Size;
            ++Itr;
        }
    }
}