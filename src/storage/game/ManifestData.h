#pragma once

#include "ManifestData.h"

namespace EGL3::Storage::Game {
    class ManifestData {
    public:
        uint32_t GetAppID() const {
            return AppID;
        }

        void SetAppID(uint32_t NewAppID) {
            AppID = NewAppID;
        }

        std::string GetLaunchExe() const {
            return std::string(LaunchExe, strnlen_s(LaunchExe, sizeof(LaunchExe)));
        }

        void SetLaunchExe(const std::string& NewLaunchExe) {
            strncpy_s(LaunchExe, NewLaunchExe.c_str(), NewLaunchExe.size());
        }

        std::string GetLaunchCommand() const {
            return std::string(LaunchCommand, strnlen_s(LaunchCommand, sizeof(LaunchCommand)));
        }

        void SetLaunchCommand(const std::string& NewLaunchCommand) {
            strncpy_s(LaunchCommand, NewLaunchCommand.c_str(), NewLaunchCommand.size());
        }

        std::string GetAppName() const {
            return std::string(AppName, strnlen_s(AppName, sizeof(AppName)));
        }

        void SetAppName(const std::string& NewAppName) {
            strncpy_s(AppName, NewAppName.c_str(), NewAppName.size());
        }

        std::string GetCloudDir() const {
            return std::string(CloudDir, strnlen_s(CloudDir, sizeof(CloudDir)));
        }

        void SetCloudDir(const std::string& NewCloudDir) {
            strncpy_s(CloudDir, NewCloudDir.c_str(), NewCloudDir.size());
        }

    private:
        char LaunchExe[256];        // Path to the executable to launch
        char LaunchCommand[256];    // Extra command arguments to add to the executable
        char AppName[64];           // Manifest-specific app name - example: FortniteReleaseBuilds
        char CloudDir[256];         // CloudDir to download missing or invalid chunks from
        uint32_t AppID;             // Manifest-specific app id - example: 1
    };

    static_assert(sizeof(ManifestData) == 836);

    static_assert(sizeof(ManifestData) < 1792); // Hard stop, don't pass this under any circumstance
}