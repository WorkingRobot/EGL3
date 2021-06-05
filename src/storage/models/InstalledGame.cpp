#include "InstalledGame.h"

#include "../../utils/Log.h"

namespace EGL3::Storage::Models {
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, InstalledGame& Val)
    {
        Stream >> Val.Path;
        Stream >> Val.Flags;
        if (Val.GetFlag<InstallFlags::SelectedIds>()) {
            Stream >> Val.SelectedIds;
        }

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const InstalledGame& Val)
    {
        Stream << Val.Path;
        Stream << Val.Flags;
        if (Val.GetFlag<InstallFlags::SelectedIds>()) {
            Stream << Val.SelectedIds;
        }

        return Stream;
    }

    bool InstalledGame::IsValid() const
    {
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

    const Storage::Game::Header* InstalledGame::GetHeader() const
    {
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

    const Storage::Game::ManifestData* InstalledGame::GetManifestData() const
    {
        if (!IsValid()) {
            return nullptr;
        }

        return std::visit([](auto&& Arg) -> const Storage::Game::ManifestData* {
            using T = std::decay_t<decltype(Arg)>;
            if constexpr (std::is_same_v<T, Storage::Game::ManifestData>)
                return Arg.GetHeader().Get();
            else if constexpr (std::is_same_v<T, MetadataInfo>)
                return &Arg.ManifestData;
            else
                return nullptr;
        }, Data);
    }

    bool InstalledGame::IsArchiveOpen() const
    {
        if (auto ArchivePtr = std::get_if<Game::Archive>(&Data)) {
            return ArchivePtr->IsValid();
        }

        return false;
    }

    Storage::Game::Archive* InstalledGame::OpenArchive()
    {
        // Already has an archive
        if (IsArchiveOpen()) {
            return &std::get<Game::Archive>(Data);
        }

        // Open archive, use only if valid (and if it exists)
        {
            Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Load);
            if (Archive.IsValid()) {
                return &Data.emplace<Storage::Game::Archive>(std::move(Archive));
            }
        }

        // Create archive, use only if valid
        {
            Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Create);
            if (Archive.IsValid()) {
                return &Data.emplace<Storage::Game::Archive>(std::move(Archive));
            }
        }

        return nullptr;
    }

    void InstalledGame::CloseArchive()
    {
        Data.emplace<std::monostate>();
    }

    bool InstalledGame::IsMounted() const
    {
        return MountData.IsMounted();
    }

    bool InstalledGame::Mount(Service::Pipe::Client& Client)
    {
        if (!MountData) {
            MountData = Service::Pipe::ServicedMount(Client, Path);
            return MountData.IsMounted();
        }
        return true;
    }

    void InstalledGame::Unmount()
    {
        MountData.Unmount();
    }

    const std::filesystem::path& InstalledGame::GetMountPath()
    {
        return MountData.GetMountPath();
    }

    const std::filesystem::path& InstalledGame::GetPath() const
    {
        return Path;
    }

    void InstalledGame::SetPath(const std::filesystem::path& NewPath)
    {
        Unmount();

        std::error_code Error;
        auto NewAbsPath = std::filesystem::absolute(NewPath, Error);
        EGL3_VERIFY(!Error, "Could not get absolute path");

        if (IsValid()) {
            std::filesystem::rename(Path, NewAbsPath, Error);
            EGL3_VERIFY(!Error, "Could not move installed game to new position");
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