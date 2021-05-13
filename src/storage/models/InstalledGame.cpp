#include "InstalledGame.h"

#include "../../utils/Assert.h"

namespace EGL3::Storage::Models {
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, InstalledGame& Val) {
        std::string Path;
        Stream >> Path;
        Val.Path = Path;
        Stream >> Val.Flags;
        if (Val.GetFlag<InstallFlags::CreateShortcut>()) {
            Stream >> Val.SelectedIds;
        }

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const InstalledGame& Val) {
        Stream << Val.Path.string();
        Stream << Val.Flags;
        if (Val.GetFlag<InstallFlags::CreateShortcut>()) {
            Stream << Val.SelectedIds;
        }

        return Stream;
    }

    bool InstalledGame::IsValid() const {
        EnsureData();

        return std::visit([](auto&& Arg) -> bool {
            using T = std::decay_t<decltype(Arg)>;
            if constexpr (std::is_same_v<T, Storage::Game::Archive>)
                return Arg.IsValid();
            else if constexpr (std::is_same_v<T, MetadataInfo>)
                return Arg.Valid;
            else
                return false;
        }, Data);
    }

    const Storage::Game::Header* InstalledGame::GetHeader() const {
        if (!IsValid()) {
            return nullptr;
        }

        return std::visit([](auto&& Arg) -> const Storage::Game::Header* {
            using T = std::decay_t<decltype(Arg)>;
            if constexpr (std::is_same_v<T, Storage::Game::Archive>)
                return Arg.GetHeader().Get();
            else if constexpr (std::is_same_v<T, MetadataInfo>)
                return &Arg.Header;
            else
                return nullptr;
        }, Data);
    }

    bool InstalledGame::IsOpenForWriting() const
    {
        if (auto ArchivePtr = std::get_if<Game::Archive>(&Data)) {
            return ArchivePtr->IsValid() && !ArchivePtr->IsReadonly();
        }

        return false;
    }

    bool InstalledGame::OpenArchiveRead() const
    {
        // Already has an archive
        if (auto ArchivePtr = std::get_if<Game::Archive>(&Data)) {
            return ArchivePtr->IsValid();
        }

        // Open archive, use only if valid
        Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Read);
        if (Archive.IsValid()) {
            Data.emplace<Storage::Game::Archive>(std::move(Archive));
            return true;
        }

        return false;
    }

    bool InstalledGame::OpenArchiveWrite()
    {
        // Already has an archive. If there is one in read mode, overwriting it will cause issues
        if (auto ArchivePtr = std::get_if<Game::Archive>(&Data)) {
            return ArchivePtr->IsValid() && !ArchivePtr->IsReadonly();
        }

        // Open archive, use only if valid (and if it exists)
        {
            Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Load);
            if (Archive.IsValid()) {
                Data.emplace<Storage::Game::Archive>(std::move(Archive));
                return true;
            }
        }

        // Create archive, use only if valid
        {
            Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Create);
            if (Archive.IsValid()) {
                Data.emplace<Storage::Game::Archive>(std::move(Archive));
                return true;
            }
        }

        return false;
    }

    void InstalledGame::CloseArchive()
    {
        Data.emplace<std::monostate>();
    }

    Storage::Game::Archive& InstalledGame::GetArchive() const
    {
        return std::get<Storage::Game::Archive>(Data);
    }

    bool InstalledGame::IsMounted() const
    {
        return MountData.has_value() && MountData->IsValid();
    }

    bool InstalledGame::Mount(Service::Pipe::Client& Client)
    {
        return MountData.emplace(Path, Client).IsValid();
    }

    void InstalledGame::Unmount()
    {
        MountData.reset();
    }

    char InstalledGame::GetDriveLetter() const
    {
        if (IsMounted()) {
            return MountData->GetDriveLetter();
        }
        return 0;
    }

    const std::filesystem::path& InstalledGame::GetPath() const
    {
        return Path;
    }

    void InstalledGame::SetPath(const std::filesystem::path& NewPath)
    {
        std::error_code Error;
        auto NewAbsPath = std::filesystem::absolute(NewPath, Error);
        EGL3_CONDITIONAL_LOG(!Error, LogLevel::Critical, "Could not get absolute path");

        if (IsValid()) {
            std::filesystem::rename(Path, NewAbsPath, Error);
            EGL3_CONDITIONAL_LOG(!Error, LogLevel::Critical, "Could not move installed game to new position");
        }

        Path = NewAbsPath;
    }

    InstallFlags InstalledGame::GetFlags() const
    {
        return Flags;
    }

    void InstalledGame::SetFlags(InstallFlags NewFlags)
    {
        Flags = NewFlags;
    }

    const std::vector<std::string>& InstalledGame::GetSelectedIds() const
    {
        return SelectedIds;
    }

    void InstalledGame::SetSelectedIds(const std::vector<std::string>& NewIds)
    {
        SelectedIds = NewIds;
    }

    void InstalledGame::EnsureData() const {
        if (std::holds_alternative<std::monostate>(Data)) {
            Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Read);
            Data.emplace<MetadataInfo>(Archive);
        }
    }
}