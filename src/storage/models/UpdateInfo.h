#pragma once

#include "../../storage/models/UpdateStatsInfo.h"
#include "../../utils/TaskPool.h"
#include "../../web/epic/bps/Manifest.h"
#include "../game/Archive.h"

namespace EGL3::Storage::Models {
    class UpdateInfo {
    public:
        UpdateInfo(Web::Epic::BPS::Manifest&& Manifest, size_t TaskCount, const std::filesystem::path& ArchivePath, Game::ArchiveMode ArchiveMode);

        void Begin();

        void Pause();

        Utils::Callback<void(const Storage::Models::UpdateStatsInfo&)> StatsUpdate;

    private:
        struct TaskArgs {
            Utils::Guid Guid;
            bool ToDownload; // true = download, false = delete
        };

        void Initialize();

        bool ExecuteTask();

        bool GetNextTask(TaskArgs& Args);

        Game::Archive Archive;
        const Web::Epic::BPS::Manifest Manifest;
        Utils::TaskPool TaskPool;

        std::mutex TaskDataMutex;
        std::vector<TaskArgs> TaskData;
    };
}