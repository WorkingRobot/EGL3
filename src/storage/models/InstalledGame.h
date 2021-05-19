#pragma once

#include "../../srv/pipe/client/ServicedMount.h"
#include "../../utils/streams/Stream.h"
#include "../game/Archive.h"

#include <variant>

namespace EGL3::Storage::Models {
    enum class InstallFlags : uint16_t {
        AutoUpdate          = 0x0001,
        SelectedIds         = 0x0002,
        DefaultSelectedIds  = 0x0004,
        CreateShortcut      = 0x0100,
    };

    template<InstallFlags Flag>
    static bool GetInstallFlag(InstallFlags Flags)
    {
        return (uint16_t)Flags & (uint16_t)Flag;
    }

    template<InstallFlags Flag>
    static InstallFlags SetInstallFlag(InstallFlags Flags, bool Val)
    {
        if (Val) {
            return InstallFlags((uint16_t)Flags | (uint16_t)Flag);
        }
        else {
            return InstallFlags((uint16_t)Flags & ~(uint16_t)Flag);
        }
    }

    struct InstalledGame {
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, InstalledGame& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const InstalledGame& Val);

        // If this points to a valid installed game
        bool IsValid() const;

        const Storage::Game::Header* GetHeader() const;

        const Storage::Game::ManifestData* GetManifestData() const;

        bool IsArchiveOpen() const;

        // Open (or create if it doesn't exist) for writing
        // Returns nullptr if the archive is invalid
        Storage::Game::Archive* OpenArchive();

        void CloseArchive();

        bool IsMounted() const;

        bool Mount(Service::Pipe::Client& Client);

        void Unmount();

        const std::filesystem::path& GetMountPath();

        const std::filesystem::path& GetPath() const;

        // If there is an installed archive at the old location, it will move it to the new location
        void SetPath(const std::filesystem::path& NewPath);

        InstallFlags GetFlags() const;

        void SetFlags(InstallFlags NewFlags);

        template<InstallFlags Flag>
        bool GetFlag() const
        {
            return GetInstallFlag<Flag>(Flags);
        }

        template<InstallFlags Flag>
        void SetFlag(bool Val)
        {
            Flags = SetInstallFlag<Flag>(Flags, Val);
        }

        const std::vector<std::string>& GetSelectedIds() const;

        void SetSelectedIds(const std::vector<std::string>& NewIds);

    private:
        // Ensures that there is at least some metadata stored about the archive at the path
        void EnsureData() const;

        std::filesystem::path Path;
        InstallFlags Flags;
        std::vector<std::string> SelectedIds;

        struct MetadataInfo {
            MetadataInfo(const Storage::Game::Archive& Archive) :
                Valid(Archive.IsValid()),
                Header(Archive.IsValid() ? *Archive.GetHeader().Get() : Storage::Game::Header{}),
                ManifestData(Archive.IsValid() ? *Archive.GetManifestData().Get() : Storage::Game::ManifestData{})
            {

            }

            bool Valid;
            Storage::Game::Header Header;
            Storage::Game::ManifestData ManifestData;
        };

        mutable std::variant<std::monostate, Storage::Game::Archive, MetadataInfo> Data;
        Service::Pipe::ServicedMount MountData;
    };
}