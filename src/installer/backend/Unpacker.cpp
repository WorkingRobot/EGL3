#include "Unpacker.h"

#include "../../utils/streams/FileStream.h"
#include "../../utils/Format.h"
#include "streams/LZ4DecompStream.h"
#include "Constants.h"
#include "RegistryInfo.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Installer::Backend {
    Unpacker::Unpacker(Utils::Streams::Stream& InStream, const std::filesystem::path& TargetDirectory) :
        InStream(InStream),
        TargetDirectory(TargetDirectory)
    {
    }

    void Unpacker::Run()
    {
        Future = std::async(std::launch::async, [this]() {
            UpdateProgress(.02f, State::StoppingService);

            CallService("stop nowait");

            UpdateProgress(.08f, State::Opening);

            uint32_t Magic;
            InStream >> Magic;
            if (Magic != FileMagic) {
                OnProgressError("Invalid magic");
                return;
            }

            Streams::LZ4DecompStream Stream(InStream);

            UpdateProgress(.1f, State::Copying);

            auto FileBuffer = std::make_unique<char[]>(BufferSize);
            while (true) {
                uint32_t FileSize;
                Stream >> FileSize;
                if (FileSize == -1) {
                    break;
                }

                std::string FileName;
                Stream >> FileName;

                auto FilePath = TargetDirectory / FileName;
                std::error_code Error;
                std::filesystem::create_directories(FilePath.parent_path(), Error);

                Utils::Streams::FileStream FileStream;
                if (!FileStream.open(FilePath, "wb")) {
                    OnProgressError(Utils::Format("Could not open file %s", FilePath.string().c_str()));
                    return;
                }

                uint32_t BytesLeft = FileSize;
                do {
                    auto BytesToCopy = std::min(BytesLeft, BufferSize);
                    Stream.read(FileBuffer.get(), BytesToCopy);
                    FileStream.write(FileBuffer.get(), BytesToCopy);
                    BytesLeft -= BytesToCopy;
                } while (BytesLeft);
            }

            UpdateProgress(.85f, State::Registry);

            RegistryInfo Registry;
            Stream >> Registry;

            if (Registry.IsInstalled()) {
                Registry.Uninstall();
            }

            if (!Registry.Install(std::filesystem::absolute(TargetDirectory))) {
                OnProgressError("Could not add registry data");
                return;
            }

            UpdateProgress(.90f, State::Shortcut);

            if (Registry.IsShortcutted()) {
                Registry.Unshortcut();
            }

            if (!Registry.Shortcut(std::filesystem::absolute(TargetDirectory), LaunchPath)) {
                OnProgressError("Could not add shortcut");
                return;
            }

            UpdateProgress(.95f, State::StartingService);

            CallService("patch nowait");

            UpdateProgress(1, State::Done);
        });
    }

    void Unpacker::GetProgress(float& Value, State& State) const
    {
        Value = Progress;
        State = CurrentState;
    }

    const std::filesystem::path& Unpacker::GetLaunchPath() const
    {
        return LaunchPath;
    }

    void Unpacker::CallService(const char* Args) const
    {
        std::error_code Error;
        if (std::filesystem::is_regular_file(TargetDirectory / "EGL3_SRV.exe", Error)) {
            STARTUPINFO StartupInfo{
                .cb = sizeof(STARTUPINFO)
            };

            auto ExePath = TargetDirectory / "EGL3_SRV.exe";
            std::string CommandLineString = Utils::Format("\"%s\" %s", ExePath.string().c_str(), Args);
            PROCESS_INFORMATION ProcInfo;

            BOOL Ret = CreateProcess(
                NULL,
                CommandLineString.data(),
                NULL, NULL,
                FALSE,
                (DWORD)(NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW),
                NULL,
                ExePath.parent_path().string().c_str(),
                &StartupInfo,
                &ProcInfo
            );
            if (Ret) {
                CloseHandle(ProcInfo.hThread);
                WaitForSingleObject(ProcInfo.hProcess, INFINITE);
                CloseHandle(ProcInfo.hProcess);
            }
        }
    }

    void Unpacker::UpdateProgress(float NewProgress, State NewState)
    {
        Progress = NewProgress;
        CurrentState = NewState;
        OnProgressUpdate();
    }
}