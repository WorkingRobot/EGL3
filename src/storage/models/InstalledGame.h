#pragma once

#include "../../utils/streams/Stream.h"
#include "../game/Archive.h"
#include "MountedGame.h"

#include <filesystem>
#include <optional>
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

        bool IsValid() const;

        const Storage::Game::Header* GetHeader() const;

        bool IsOpenForWriting() const;

        // Open for reading
        // Returns false if the archive is invalid (or does not exist)
        bool OpenArchiveRead() const;

        // Open (or create if it doesn't exist) for writing
        // Returns false if the archive is invalid or is already opened in read mode
        bool OpenArchiveWrite();

        void CloseArchive();

        // Only run if OpenArchive...() returns true
        Storage::Game::Archive& GetArchive() const;

        bool IsMounted() const;

        bool Mount(Service::Pipe::Client& Client);

        void Unmount();

        char GetDriveLetter() const;

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
        void EnsureData() const;

        std::filesystem::path Path;
        InstallFlags Flags;
        std::vector<std::string> SelectedIds;

        struct MetadataInfo {
            MetadataInfo(const Storage::Game::Archive& Archive) :
                Valid(Archive.IsValid()),
                Header(Archive.IsValid() ? *Archive.GetHeader().Get() : Storage::Game::Header{})
            {

            }

            bool Valid;
            Storage::Game::Header Header;
        };

        mutable std::variant<std::monostate, Storage::Game::Archive, MetadataInfo> Data;
        std::optional<MountedGame> MountData;
    };
}