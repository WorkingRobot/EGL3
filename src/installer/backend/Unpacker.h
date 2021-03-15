#pragma once

#include "../../utils/streams/Stream.h"
#include "../../utils/Callback.h"

#include <filesystem>
#include <future>

namespace EGL3::Installer::Backend {
    class Unpacker {
    public:
        enum class State : uint8_t {
            Opening,
            Copying,
            Registry,
            Shortcut,
            Done
        };

        Unpacker(Utils::Streams::Stream& InStream, const std::filesystem::path& TargetDirectory);
        
        void Run();

        void GetProgress(float& Value, State& State) const;

        const std::filesystem::path& GetLaunchPath() const;

        Utils::Callback<void()> OnProgressUpdate;
        Utils::Callback<void(const std::string& Error)> OnProgressError;

    private:
        void UpdateProgress(float NewProgress, State NewState);

        std::future<void> Future;

        Utils::Streams::Stream& InStream;
        const std::filesystem::path& TargetDirectory;

        float Progress;
        State CurrentState;
        std::filesystem::path LaunchPath;
    };
}