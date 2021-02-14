#pragma once

#include "../../utils/streams/Stream.h"
#include "../game/Archive.h"

#include <filesystem>
#include <variant>

namespace EGL3::Storage::Models {
    class InstalledGame {
        struct MetadataInfo {
            MetadataInfo(const Storage::Game::Archive& Archive) :
                Valid(Archive.IsValid()),
                Header(Archive.IsValid() ? *Archive.GetHeader().Get() : Storage::Game::Header{})
            {

            }

            bool Valid;
            Storage::Game::Header Header;
        };

    public:
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, InstalledGame& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const InstalledGame& Val);

        bool IsValid() const;

        const Storage::Game::Header* GetHeader() const;

        // Open for reading
        // Returns false if the archive is invalid (or does not exist)
        bool OpenArchiveRead() const;

        // Open (or create if it doesn't exist) for writing
        // Returns false if the archive is invalid or is already opened in read mode
        bool OpenArchiveWrite();

        void CloseArchive();

        // Only run if OpenArchive...() returns true
        Storage::Game::Archive& GetArchive() const;

        const std::filesystem::path& GetPath() const;

        // If there is an installed archive at the old location, it will move it to the new location
        void SetPath(const std::filesystem::path& NewPath);

        bool GetAutoUpdate() const;

        void SetAutoUpdate(bool Val);

        bool GetCreateShortcut() const;

        void SetCreateShortcut(bool Val);

    private:
        void EnsureData() const;

        std::filesystem::path Path;
        bool AutoUpdate;
        bool CreateShortcut;

        mutable std::variant<std::monostate, Storage::Game::Archive, MetadataInfo> Data;
    };
}