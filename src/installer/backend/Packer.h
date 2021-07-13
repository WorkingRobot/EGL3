#pragma once

#include <filesystem>

namespace EGL3::Installer::Backend {
    class Packer {
    public:
        Packer(const std::string& AppVersion, const std::string& ShortAppVersion, uint32_t VersionMajor, uint32_t VersionMinor, uint32_t VersionPatch, uint64_t VersionNum);

        void SetInputPath(const std::filesystem::path& InputPath);

        bool Pack(const std::filesystem::path& OutputPath) const;

        bool ExportJson(const std::filesystem::path& OutputPath, const std::filesystem::path& PatchNotesInput) const;

    private:
        std::filesystem::path InputPath;

        std::string AppVersion;
        std::string ShortAppVersion;
        uint32_t VersionMajor;
        uint32_t VersionMinor;
        uint32_t VersionPatch;
        uint64_t VersionNum;
    };
}