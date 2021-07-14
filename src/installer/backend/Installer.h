#pragma once

#include "../../utils/Callback.h"

#include <filesystem>
#include <future>

namespace EGL3::Installer::Backend {
    enum class InstallState : uint8_t {
        Connecting,
        StoppingService,
        Copying,
        Registry,
        Shortcut,
        StartingService,
        Done
    };

    class Installer {
    public:
        Installer();

        // Begin the actual installation process
        void Start();

        // After "Finish" launch the program
        bool LaunchProgram() const;

        const std::filesystem::path& GetInstallPath() const;

        void SetInstallPath(const std::filesystem::path& NewPath);

        Utils::Callback<void(InstallState State, float Progress, const std::string& Text)> OnProgressUpdate;
        Utils::Callback<void(const std::string& Error)> OnError;

    private:
        std::filesystem::path InstallPath;

        std::future<void> Task;

        std::string AppVersion;
        std::string ShortAppVersion;
        uint32_t VersionMajor;
        uint32_t VersionMinor;
        uint32_t VersionPatch;
        uint64_t VersionNum;
        uint64_t InstalledSize;

        std::filesystem::path LaunchPath;

        void Run();

        bool StopService() const;

        bool PatchService() const;
    };
}