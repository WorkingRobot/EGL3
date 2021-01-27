#include "UpdateInfo.h"

namespace EGL3::Storage::Models {
    UpdateInfo::UpdateInfo(Web::Epic::BPS::Manifest&& _Manifest, size_t TaskCount, const std::filesystem::path& ArchivePath, Game::ArchiveMode ArchiveMode) :
        Archive(ArchivePath, ArchiveMode),
        Manifest(std::move(_Manifest)),
        TaskPool(TaskCount)
    {
        TaskPool.Task.Set([this]() { return ExecuteTask(); });

        std::vector<Utils::Guid> ArchiveChunks;
        //Archive.GetAllInstalledChunkGuids(ArchiveChunks);
        
        std::vector<Utils::Guid> ManifestChunks;
        ManifestChunks.reserve(Manifest.ChunkDataList.ChunkList.size());
        for (auto& Chunk : Manifest.ChunkDataList.ChunkList) {
            ManifestChunks.emplace_back(Chunk.Guid);
        }

        std::sort(ArchiveChunks.begin(), ArchiveChunks.end());
        std::sort(ManifestChunks.begin(), ManifestChunks.end());

        std::vector<Utils::Guid> UpdatedChunks;
        std::vector<Utils::Guid> DeletedChunks;

        std::set_difference(ManifestChunks.begin(), ManifestChunks.end(), ArchiveChunks.begin(), ArchiveChunks.end(), std::back_inserter(UpdatedChunks));
        std::set_difference(ArchiveChunks.begin(), ArchiveChunks.end(), ManifestChunks.begin(), ManifestChunks.end(), std::back_inserter(DeletedChunks));

        TaskData.reserve(DeletedChunks.size() + UpdatedChunks.size());

        std::transform(std::make_move_iterator(UpdatedChunks.begin()), std::make_move_iterator(UpdatedChunks.end()), std::back_inserter(TaskData), [](Utils::Guid&& Guid) {
            return TaskArgs{
                .Guid = std::move(Guid),
                .ToDownload = true
            };
        });
        std::transform(std::make_move_iterator(DeletedChunks.begin()), std::make_move_iterator(DeletedChunks.end()), std::back_inserter(TaskData), [](Utils::Guid&& Guid) {
            return TaskArgs{
                .Guid = std::move(Guid),
                .ToDownload = false
            };
        });
    }

    void UpdateInfo::Begin()
    {
        TaskPool.Begin();
    }

    void UpdateInfo::Pause()
    {
        TaskPool.Pause();
    }

    bool UpdateInfo::ExecuteTask()
    {
        TaskArgs Args;
        if (!GetNextTask(Args)) {
            return false;
        }
        printf("%s %08X%08X%08X%08X\n", Args.ToDownload ? "Downloading" : "Deleting", Args.Guid.A, Args.Guid.B, Args.Guid.C, Args.Guid.D);
        return true;
    }

    bool UpdateInfo::GetNextTask(TaskArgs& Args)
    {
        std::lock_guard Lock(TaskDataMutex);

        if (TaskData.empty()) {
            return false;
        }
        Args = TaskData.back();
        TaskData.pop_back();
        return true;
    }
}