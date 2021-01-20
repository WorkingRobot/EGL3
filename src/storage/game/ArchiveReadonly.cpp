#include "ArchiveReadonly.h"

#include "../../utils/Assert.h"

namespace EGL3::Storage::Game {
    ArchiveReadonly::ArchiveReadonly(const std::filesystem::path& Path) noexcept :
        ArchiveReadonly()
    {
        if (!EGL3_CONDITIONAL_LOG(std::filesystem::is_regular_file(Path), LogLevel::Error, "Archive file does not exist")) {
            return;
        }

        Backend = std::make_unique<Utils::Mmio::MmioFileReadonly>(Path);
        if (!EGL3_CONDITIONAL_LOG(GetBackend().IsValid(), LogLevel::Error, "Archive file could not be opened")) {
            return;
        }

        if (!EGL3_CONDITIONAL_LOG(GetBackend().IsValidPosition(Header::GetMinimumArchiveSize()), LogLevel::Error, "Archive file is too small")) {
            return;
        }

        if (!EGL3_CONDITIONAL_LOG(GetHeader().HasValidMagic(), LogLevel::Error, "Archive has invalid magic")) {
            return;
        }

        if (!EGL3_CONDITIONAL_LOG(GetHeader().HasValidHeaderSize(), LogLevel::Error, "Archive header has invalid size")) {
            return;
        }

        if (!EGL3_CONDITIONAL_LOG(GetHeader().GetVersion() == ArchiveVersion::Latest, LogLevel::Error, "Archive has invalid version")) {
            return;
        }

        Valid = true;
    }

    bool ArchiveReadonly::IsValid() const
    {
        return false;
    }

    ArchiveReadonly::ArchiveReadonly() :
        Valid(false)
    {

    }

    const Utils::Mmio::MmioFileReadonly& ArchiveReadonly::GetBackend() const
    {
        return *Backend;
    }

    const Header& ArchiveReadonly::GetHeader() const
    {
        return *(const Header*)(GetBackend().Get() + 0);
    }

    const ManifestData& ArchiveReadonly::GetManifestData() const
    {
        return *(const ManifestData*)(GetBackend().Get() + GetHeader().GetOffsetManifestData());
    }

    const ArchiveReadonly::RunlistFile& ArchiveReadonly::GetRunlistFile() const
    {
        return *(const RunlistFile*)(GetBackend().Get() + GetHeader().GetOffsetRunlistFile());
    }

    const ArchiveReadonly::RunlistChunk& ArchiveReadonly::GetRunlistChunkPart() const
    {
        return *(const RunlistChunk*)(GetBackend().Get() + GetHeader().GetOffsetRunlistChunkPart());
    }

    const ArchiveReadonly::RunlistChunk& ArchiveReadonly::GetRunlistChunkInfo() const
    {
        return *(const RunlistChunk*)(GetBackend().Get() + GetHeader().GetOffsetRunlistChunkInfo());
    }

    const ArchiveReadonly::RunlistChunk& ArchiveReadonly::GetRunlistChunkData() const
    {
        return *(const RunlistChunk*)(GetBackend().Get() + GetHeader().GetOffsetRunlistChunkData());
    }

    const ArchiveReadonly::RunIndexBase& ArchiveReadonly::GetRunIndex() const
    {
        return *(const RunIndexBase*)(GetBackend().Get() + GetHeader().GetOffsetRunIndex());
    }
}
