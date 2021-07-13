#include "Installer.h"

#include "../../utils/formatters/Path.h"
#include "../../utils/streams/FileStream.h"
#include "../../utils/KnownFolders.h"
#include "streams/LZ4DecompStream.h"
#include "streams/WebStream.h"
#include "Constants.h"
#include "RegistryInfo.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

namespace EGL3::Installer::Backend {
    Installer::Installer() :
        InstallPath(Utils::Platform::GetKnownFolderPath(FOLDERID_ProgramFiles) / "EGL3")
    {
        
    }

    void Installer::Start()
    {
        Task = std::async(std::launch::async, &Installer::Run, this);
    }

    bool Installer::LaunchProgram() const
    {
        // https://docs.microsoft.com/en-us/archive/blogs/aaron_margosis/faq-how-do-i-start-a-program-as-the-desktop-user-from-an-elevated-app
        // Scroll all the way down to EricLaw's comment
        std::filesystem::path ExplorerPath;
        ExplorerPath = Utils::Platform::GetKnownFolderPath(FOLDERID_Windows) / "explorer.exe";
        ShellExecute(NULL, "open", ExplorerPath.string().c_str(), LaunchPath.string().c_str(), NULL, SW_SHOWNORMAL);
        return true;
    }

    const std::filesystem::path& Installer::GetInstallPath() const
    {
        return InstallPath;
    }

    void Installer::SetInstallPath(const std::filesystem::path& NewPath)
    {
        InstallPath = NewPath;
    }

    void Installer::Run()
    {
        OnProgressUpdate(InstallState::Connecting, .00f, "");

        Streams::WebStream UpdateStream(L"epic.gl", std::format(L"/updates/{}.eglu", InstallVersion));
        if (!UpdateStream.valid()) {
            OnError("Could not open a connection to epic.gl");
            return;
        }

        {
            uint32_t Magic;
            UpdateStream >> Magic;
            if (Magic != FileMagic) {
                OnError("Invalid file magic, maybe the update doesn't exist?");
                return;
            }
        }

        UpdateStream >> AppVersion;
        UpdateStream >> ShortAppVersion;
        UpdateStream >> VersionMajor;
        UpdateStream >> VersionMinor;
        UpdateStream >> VersionPatch;
        UpdateStream >> VersionNum;

        uint32_t FileCount;
        UpdateStream >> FileCount;

        OnProgressUpdate(InstallState::StoppingService, .05f, "");

        if (!StopService()) {
            OnError("Could not stop service");
            return;
        }

        OnProgressUpdate(InstallState::Copying, .10f, "");

        Streams::LZ4DecompStream Stream(UpdateStream);

        auto FileBuffer = std::make_unique<char[]>(BufferSize);
        for (uint32_t Idx = 0; Idx < FileCount; ++Idx) {
            uint64_t FileSize;
            Stream >> FileSize;

            InstalledSize += FileSize;
            
            std::string FileName;
            Stream >> FileName;

            auto FilePath = InstallPath / FileName;
            {
                std::error_code Code;
                std::filesystem::create_directories(FilePath.parent_path(), Code);
            }

            OnProgressUpdate(InstallState::Copying, .10f + ((float)Idx / FileCount) * .80f, std::format("({}/{}) {}", Idx + 1, FileCount, FilePath));
            Utils::Streams::FileStream FileStream;
            if (!FileStream.open(FilePath, "wb")) {
                OnError(std::format("Could not open file {}", FilePath));
                return;
            }

            uint64_t BytesLeft = FileSize;
            while (BytesLeft) {
                auto BytesToCopy = std::min(BytesLeft, BufferSize);
                Stream.read(FileBuffer.get(), BytesToCopy);
                FileStream.write(FileBuffer.get(), BytesToCopy);
                BytesLeft -= BytesToCopy;
            }
        }

        OnProgressUpdate(InstallState::Registry, .90f, "");

        RegistryInfo Reg{
            .ProductGuid = "EGL3",
            .LaunchExe = "EGL3.exe",
            .DisplayIcon = "EGL3.exe",
            .DisplayName = "EGL3",
            .DisplayVersion = ShortAppVersion,
            .EstimatedSize = uint32_t(InstalledSize >> 10),
            .HelpLink = "https://epic.gl/discord",
            .Publisher = "WorkingRobot",
            .UninstallString = "uninstall.exe",
            .URLInfoAbout = "https://epic.gl",
            .URLUpdateInfo = "https://epic.gl/changelog",
            .VersionMajor = VersionMajor,
            .VersionMinor = VersionMinor
        };

        if (Reg.IsInstalled()) {
            Reg.Uninstall();
        }
        if (!Reg.Install(InstallPath)) {
            OnError("Could not update registry");
        }

        OnProgressUpdate(InstallState::Shortcut, .925f, "");

        if (Reg.IsShortcutted()) {
            Reg.Unshortcut();
        }
        if (!Reg.Shortcut(InstallPath, LaunchPath)) {
            OnError("Could not create shortcut");
        }

        OnProgressUpdate(InstallState::StartingService, .95f, "");

        if (!PatchService()) {
            OnError("Could not start service");
        }

        OnProgressUpdate(InstallState::Done, 1.f, "");
    }

    bool Installer::StopService() const
    {
        HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        PROCESSENTRY32 Entry{
            .dwSize = sizeof(Entry)
        };
        BOOL Result = Process32First(Snapshot, &Entry);
        while (Result)
        {
            if (strcmp(Entry.szExeFile, "EGL3_SRV") == 0)
            {
                HANDLE Process = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)Entry.th32ProcessID);
                if (Process != NULL)
                {
                    auto Ret = TerminateProcess(Process, 1);
                    CloseHandle(Process);

                    CloseHandle(Snapshot);
                    return Ret;
                }
            }
            Result = Process32Next(Snapshot, &Entry);
        }

        CloseHandle(Snapshot);
        return true;
    }

    bool Installer::PatchService() const
    {
        auto SrvPath = InstallPath / "EGL3_SRV.exe";

        std::error_code Error;
        if (!std::filesystem::is_regular_file(SrvPath, Error)) {
            return false;
        }

        STARTUPINFO StartupInfo{
            .cb = sizeof(StartupInfo),
            .dwFlags = STARTF_USESHOWWINDOW,
            .wShowWindow = SW_HIDE
        };

        auto CommandLineString = std::format("\"{}\" {}", SrvPath, "patch nowait");
        PROCESS_INFORMATION ProcInfo;

        BOOL Ret = CreateProcess(
            NULL,
            CommandLineString.data(),
            NULL, NULL,
            FALSE,
            (DWORD)(NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW),
            NULL,
            NULL,
            &StartupInfo,
            &ProcInfo
        );
        if (Ret) {
            CloseHandle(ProcInfo.hThread);
            WaitForSingleObject(ProcInfo.hProcess, INFINITE);
            CloseHandle(ProcInfo.hProcess);
        }

        return Ret;
    }
}