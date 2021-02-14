#include "InstalledGame.h"

#include "../../utils/Assert.h"

namespace EGL3::Storage::Models {
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, InstalledGame& Val) {
        std::string Path;
        Stream >> Path;
        Val.Path = Path;
        Stream >> Val.AutoUpdate;
        Stream >> Val.CreateShortcut;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const InstalledGame& Val) {
        Stream << Val.Path.string();
        Stream << Val.AutoUpdate;
        Stream << Val.CreateShortcut;

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

    bool InstalledGame::GetAutoUpdate() const
    {
        return AutoUpdate;
    }

    void InstalledGame::SetAutoUpdate(bool Val)
    {
        AutoUpdate = Val;
    }

    bool InstalledGame::GetCreateShortcut() const
    {
        return CreateShortcut;
    }

    void InstalledGame::SetCreateShortcut(bool Val)
    {
        CreateShortcut = Val;
    }

    void InstalledGame::EnsureData() const {
        if (std::holds_alternative<std::monostate>(Data)) {
            Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Read);
            Data.emplace<MetadataInfo>(Archive);
        }
    }
}