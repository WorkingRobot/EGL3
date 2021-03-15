#include "Unpacker.h"

#include "../../utils/streams/FileStream.h"
#include "streams/LZ4DecompStream.h"
#include "Constants.h"
#include "RegistryInfo.h"

namespace EGL3::Installer::Backend {
    Unpacker::Unpacker(Utils::Streams::Stream& InStream, const std::filesystem::path& TargetDirectory) :
        InStream(InStream),
        TargetDirectory(TargetDirectory)
    {
    }

    void Unpacker::Run()
    {
        Future = std::async(std::launch::async, [this]() {
            UpdateProgress(.02f, State::Opening);

            uint32_t Magic;
            InStream >> Magic;
            if (Magic != FileMagic) {
                OnProgressError("Invalid magic");
                return;
            }

            Streams::LZ4DecompStream Stream(InStream);

            UpdateProgress(.05f, State::Copying);

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
                std::filesystem::create_directories(FilePath.parent_path());

                Utils::Streams::FileStream FileStream;
                if (!FileStream.open(FilePath, "wb")) {
                    OnProgressError("Could not open file");
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

            UpdateProgress(.90f, State::Registry);

            RegistryInfo Registry;
            Stream >> Registry;

            if (Registry.IsInstalled()) {
                Registry.Uninstall();
            }

            if (!Registry.Install(std::filesystem::absolute(TargetDirectory))) {
                OnProgressError("Could not add registry data");
                return;
            }

            UpdateProgress(.95f, State::Shortcut);

            if (Registry.IsShortcutted()) {
                Registry.Unshortcut();
            }

            if (!Registry.Shortcut(std::filesystem::absolute(TargetDirectory), LaunchPath)) {
                OnProgressError("Could not add shortcut");
                return;
            }

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

    void Unpacker::UpdateProgress(float NewProgress, State NewState)
    {
        Progress = NewProgress;
        CurrentState = NewState;
        OnProgressUpdate();
    }
}