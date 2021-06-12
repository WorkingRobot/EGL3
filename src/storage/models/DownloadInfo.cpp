#include "DownloadInfo.h"

#include "../../modules/Game/GameInfo.h"
#include "../../web/epic/EpicClient.h"
#include "../../utils/Align.h"
#include "../../utils/Config.h"
#include "../../utils/Taskbar.h"

#include <charconv>
#include <regex>

namespace EGL3::Storage::Models {
    static constexpr std::chrono::milliseconds RefreshTime(500);
    static constexpr double DivideRate = std::chrono::duration_cast<std::chrono::duration<double>>(RefreshTime) / std::chrono::seconds(1);

    DownloadInfo::DownloadInfo(Game::GameId Id, const InstalledGamesRequest& GetInstalledGames) :
        Id(Id),
        GameConfig(nullptr),
        GetInstalledGames(GetInstalledGames),
        CurrentState(DownloadInfoState::Options),
        StateData(StateOptions(Id))
    {
        auto& Data = GetStateData<StateOptions>();

        auto& Games = GetInstalledGames();
        auto ConfigPtr = std::find_if(Games.begin(), Games.end(), [this](const InstalledGame& Game) {
            return Game.IsValid() && Game.GetHeader()->GetGameId() == this->Id;
        });
        if (ConfigPtr != Games.end()) {
            GameConfig = &*ConfigPtr;
            Data.DefaultArchivePath = Data.ArchivePath = GameConfig->GetPath();
            Data.Flags = GameConfig->GetFlags();
            Data.SelectedIds = GameConfig->GetSelectedIds();
        }
        else {
            Data.DefaultArchivePath = Data.ArchivePath = Utils::Config::GetFolder() / "Fortnite.egia";
            Data.Flags = InstallFlags((uint16_t)InstallFlags::AutoUpdate | (uint16_t)InstallFlags::CreateShortcut | (uint16_t)InstallFlags::DefaultSelectedIds);
        }
    }

    DownloadInfo::~DownloadInfo()
    {
        if (std::holds_alternative<StateInstalling>(StateData)) {
            SetDownloadCancelled();
        }

        if (PrimaryTask.valid()) {
            PrimaryTask.get();
        }
    }

    DownloadInfoState DownloadInfo::GetState() const
    {
        return CurrentState;
    }

    void DownloadInfo::SetState(DownloadInfoState NewState)
    {
        OnStateUpdate.emit(CurrentState = NewState);
    }

    void DownloadInfo::CancelDownloadSetup()
    {
        StateData.emplace<StateCancelled>();
        SetState(DownloadInfoState::Cancelled);
    }

    template<typename Comparer>
    struct GuidComparer {
        const Game::ArchiveList<Game::RunlistId::ChunkInfo>& ArchiveChunks;

        GuidComparer(const Game::ArchiveList<Game::RunlistId::ChunkInfo>& ArchiveChunks) :
            ArchiveChunks(ArchiveChunks)
        {

        }

        template<class T>
        const Utils::Guid& Convert(const T& Val) const = delete;

        template<>
        const Utils::Guid& Convert<uint32_t>(const uint32_t& Val) const {
            return ArchiveChunks[Val].Guid;
        }

        template<>
        const Utils::Guid& Convert<std::reference_wrapper<const Utils::Guid>>(const std::reference_wrapper<const Utils::Guid>& Val) const {
            return Val;
        }

        template<class TA, class TB>
        bool operator()(const TA& A, const TB& B) const {
            return Comparer{}(Convert(A), Convert(B));
        }
    };

    void DownloadInfo::BeginDownload(const LatestManifestRequest& GetLatestManifest)
    {
        PrimaryTask = std::async(std::launch::async, [GetLatestManifest, this]() {
            {
                if (!GameConfig) {
                    GameConfig = &GetInstalledGames().emplace_back();
                }
                auto& Data = GetStateData<StateOptions>();
                GameConfig->SetPath(Data.ArchivePath);
                GameConfig->SetFlags(Data.Flags);
                GameConfig->SetSelectedIds(Data.SelectedIds);

                // Move install tags to a temporary since we can't emplace a variant
                // with arguments that are already replaced by the new type
                std::vector<std::string> InstallTags(std::move(Data.InstallTags));
                Utils::EGL::ChunkProvider EGLProvider(std::move(Data.EGLProvider));
                StateData.emplace<StateInitializing>(std::move(InstallTags), std::move(EGLProvider));
            }

            std::chrono::steady_clock::time_point BeginTimestamp = std::chrono::steady_clock::now();

            {
                auto& Data = GetStateData<StateInitializing>();
                SetState(DownloadInfoState::Initializing);

                auto Archive = GameConfig->OpenArchive();
                if (Archive == nullptr) {
                    EGL3_LOG(LogLevel::Error, "Couldn't open for writing");
                    return;
                }

                std::optional<Web::Epic::BPS::Manifest> Manifest;
                std::string CloudDir;
                do {
                    auto Resp = GetLatestManifest(Id, CloudDir);
                    if (!Resp.HasError() && !Resp->HasError()) {
                        Manifest.emplace(std::move(Resp.Get()));
                    }
                } while (!Manifest.has_value());

                const Game::ArchiveList<Game::RunlistId::ChunkInfo> ChunkInfo(*Archive);

                // Get a simple sequence from 0...ChunkInfo-1
                std::vector<uint32_t> ArchiveGuids;
                ArchiveGuids.reserve(ChunkInfo.size());
                for (uint32_t i = 0; i < ChunkInfo.size(); ++i) {
                    ArchiveGuids.emplace_back(i);
                }

                // Get all files that will be installed
                std::vector<std::reference_wrapper<const Web::Epic::BPS::FileManifest>> ManifestFiles;
                for (auto& File : Manifest->FileManifestList.FileList) {
                    if (File.InstallTags.empty() || std::find_first_of(File.InstallTags.begin(), File.InstallTags.end(), Data.InstallTags.begin(), Data.InstallTags.end()) != File.InstallTags.end()) {
                        ManifestFiles.emplace_back(File);
                    }
                }

                // Get all required chunks that need to be installed
                std::vector<std::reference_wrapper<const Utils::Guid>> ManifestGuids;
                for (const Web::Epic::BPS::FileManifest& File : ManifestFiles) {
                    for (auto& Part : File.ChunkParts) {
                        ManifestGuids.emplace_back(Part.Guid);
                    }
                }

                // Get all chunks to be updated/added to the archive and all that need to be removed/replaced
                GuidComparer<std::less<Utils::Guid>> Comparer(ChunkInfo);

                std::sort(ArchiveGuids.begin(), ArchiveGuids.end(), Comparer);
                std::sort(ManifestGuids.begin(), ManifestGuids.end(), Comparer);
                // std::unique only removes consecutive copies (different chunk parts using the same chunk), so it has to be sorted first.
                // It also doesn't remove anything, so we have to erase it manually
                ManifestGuids.erase(std::unique(ManifestGuids.begin(), ManifestGuids.end(), GuidComparer<std::equal_to<Utils::Guid>>(ChunkInfo)), ManifestGuids.end());

                std::vector<std::reference_wrapper<const Utils::Guid>> UpdatedGuids;
                std::vector<uint32_t> DeletedChunkIdxs;

                std::set_difference(ManifestGuids.begin(), ManifestGuids.end(), ArchiveGuids.begin(), ArchiveGuids.end(), std::back_inserter(UpdatedGuids), Comparer);
                std::set_difference(ArchiveGuids.begin(), ArchiveGuids.end(), ManifestGuids.begin(), ManifestGuids.end(), std::back_inserter(DeletedChunkIdxs), Comparer);

                std::vector<std::reference_wrapper<const Web::Epic::BPS::ChunkInfo>> UpdatedChunks;
                for (auto& Guid : UpdatedGuids) {
                    auto Chunk = Manifest->GetChunk(Guid);
                    if (Chunk) {
                        UpdatedChunks.emplace_back(*Chunk);
                    }
                }

                Utils::EGL::ChunkProvider EGLProvider(std::move(Data.EGLProvider));
                StateData.emplace<StateInstalling>(std::move(CloudDir), std::move(Manifest.value()), std::move(ManifestFiles), *Archive, std::move(UpdatedChunks), std::move(DeletedChunkIdxs), std::move(EGLProvider));
            }

            {
                auto& Data = GetStateData<StateInstalling>();
                SetState(DownloadInfoState::Installing);

                Game::UpdateInfo OldUpdateInfo(Data.Archive.GetHeader()->GetUpdateInfo());

                InitializeArchive(Id, Data.Manifest.ManifestMeta, Data.CloudDir, Data.Archive.GetHeader(), Data.Archive.GetManifestData());

                // TODO: move this into the "finalizing", it's not of any use during the update anyway
                {
                    Data.ArchiveFiles.resize(Data.ManifestFiles.size());
                    uint64_t TotalChunkParts = std::accumulate(Data.ManifestFiles.begin(), Data.ManifestFiles.end(), 0ull,
                        [](uint64_t Val, const Web::Epic::BPS::FileManifest& File) { return Val + File.ChunkParts.size(); });
                    Data.ArchiveChunkParts.resize(TotalChunkParts);

                    uint32_t NextAvailableChunkPartIdx = 0;
                    uint32_t i = 0;
                    for (const Web::Epic::BPS::FileManifest& File : Data.ManifestFiles) {
                        auto& ArchiveFile = Data.ArchiveFiles[i++];
                        strncpy_s(ArchiveFile.Filename, File.Filename.c_str(), File.Filename.size());
                        memcpy(ArchiveFile.SHA, File.FileHash, 20);
                        ArchiveFile.FileSize = File.FileSize;
                        ArchiveFile.ChunkPartDataStartIdx = NextAvailableChunkPartIdx;
                        ArchiveFile.ChunkPartDataSize = File.ChunkParts.size();

                        uint32_t j = 0;
                        for (auto& ChunkPart : File.ChunkParts) {
                            auto& ArchiveChunkPart = Data.ArchiveChunkParts[(uint64_t)ArchiveFile.ChunkPartDataStartIdx + j++];
                            ArchiveChunkPart.ChunkIdx = 0;
                            ArchiveChunkPart.Offset = ChunkPart.Offset;
                            ArchiveChunkPart.Size = ChunkPart.Size;
                        }

                        NextAvailableChunkPartIdx += File.ChunkParts.size();
                    }
                }

                Data.ArchiveChunkInfos.reserve(Data.Manifest.ChunkDataList.ChunkList.size());

                Data.Pool.Task.Set([this]() { return InstallOne(); });
                Data.Pool.SetRunning();

                if (OldUpdateInfo.IsUpdating && OldUpdateInfo.TargetVersion == Data.Archive.GetHeader()->GetVersionNum()) {
                    BeginTimestamp -= std::chrono::nanoseconds(OldUpdateInfo.NanosecondsElapsed);

                    Data.PiecesTotal += OldUpdateInfo.PiecesComplete;
                    Data.PiecesComplete.fetch_add(OldUpdateInfo.PiecesComplete);

                    Data.DownloadTotal += OldUpdateInfo.BytesDownloadTotal;
                    Data.BytesDownloadTotalLast = OldUpdateInfo.BytesDownloadTotal;
                    Data.BytesDownloadTotal.fetch_add(OldUpdateInfo.BytesDownloadTotal);
                }

                auto NextUpdateTime = std::chrono::steady_clock::now();
                do {
                    std::chrono::nanoseconds PauseTime(0);

                    if (Data.Pool.GetState() == Utils::TaskPool::PoolState::Stopped) {
                        auto PauseStartTime = std::chrono::steady_clock::now();
                        Data.Pool.WaitUntilResumed();
                        auto PauseResumeTime = std::chrono::steady_clock::now();

                        PauseTime = PauseResumeTime - PauseStartTime;
                        BeginTimestamp += PauseTime;
                    }

                    uint32_t PiecesComplete = Data.PiecesComplete.load(std::memory_order::relaxed);
                    uint64_t BytesDownloadTotal = Data.BytesDownloadTotal.load(std::memory_order::relaxed);
                    uint64_t BytesReadTotal = Data.BytesReadTotal.load(std::memory_order::relaxed);
                    uint64_t BytesWriteTotal = Data.BytesWriteTotal.load(std::memory_order::relaxed);

                    DownloadInfoStats Stats{
                        .State = CurrentState,
                        .NanosecondStartTimestamp = (uint64_t)BeginTimestamp.time_since_epoch().count(),
                        .NanosecondCurrentTimestamp = (uint64_t)NextUpdateTime.time_since_epoch().count(),
                        .PiecesTotal = Data.PiecesTotal,
                        .DownloadTotal = Data.DownloadTotal,
                        .PiecesComplete = PiecesComplete,
                        .BytesDownloadTotal = BytesDownloadTotal,
                        .BytesDownloadRate = uint64_t((BytesDownloadTotal - Data.BytesDownloadTotalLast) / DivideRate),
                        .BytesReadRate = uint64_t((BytesReadTotal - Data.BytesReadTotalLast) / DivideRate),
                        .BytesWriteRate = uint64_t((BytesWriteTotal - Data.BytesWriteTotalLast) / DivideRate),
                    };
                    Data.BytesDownloadTotalLast = BytesDownloadTotal;
                    Data.BytesReadTotalLast = BytesReadTotal;
                    Data.BytesWriteTotalLast = BytesWriteTotal;
                    OnStatsUpdate(Stats);

                    Data.Archive.GetHeader()->GetUpdateInfo().NanosecondsElapsed = Stats.NanosecondCurrentTimestamp - Stats.NanosecondStartTimestamp;
                    Data.Archive.GetHeader()->GetUpdateInfo().PiecesComplete = Stats.PiecesComplete;
                    Data.Archive.GetHeader()->GetUpdateInfo().BytesDownloadTotal = Stats.BytesDownloadTotal;

                    NextUpdateTime += PauseTime + RefreshTime;
                } while (!Data.Pool.WaitUntilFinished(NextUpdateTime));

                {
                    // These aren't relaxed since we need the most up to date values
                    uint32_t PiecesComplete = Data.PiecesComplete.load();
                    uint64_t BytesDownloadTotal = Data.BytesDownloadTotal.load();

                    Data.Archive.GetHeader()->GetUpdateInfo().NanosecondsElapsed = (uint64_t)(NextUpdateTime - BeginTimestamp).count();
                    Data.Archive.GetHeader()->GetUpdateInfo().PiecesComplete = PiecesComplete;
                    Data.Archive.GetHeader()->GetUpdateInfo().BytesDownloadTotal = BytesDownloadTotal;
                }

                if (!Data.Cancelled) {
                    SetState(DownloadInfoState::Finishing);

                    std::unordered_map<std::reference_wrapper<const Utils::Guid>, uint32_t, std::hash<Utils::Guid>, std::equal_to<Utils::Guid>> ChunkLookup;
                    ChunkLookup.reserve(Data.ArchiveChunkInfos.size());

                    uint32_t i = 0;
                    for (auto& Chunk : Data.ArchiveChunkInfos) {
                        ChunkLookup.emplace(Chunk.Guid, i++);
                    }

                    i = 0;
                    for (const Web::Epic::BPS::FileManifest& File : Data.ManifestFiles) {
                        auto& ArchiveFile = Data.ArchiveFiles[i++];

                        uint32_t j = 0;
                        for (auto& ChunkPart : File.ChunkParts) {
                            auto& ArchiveChunkPart = Data.ArchiveChunkParts[(uint64_t)ArchiveFile.ChunkPartDataStartIdx + j++];
                            ArchiveChunkPart.ChunkIdx = ChunkLookup.at(ChunkPart.Guid);
                        }
                    }

                    Data.Archive.GetHeader()->GetUpdateInfo().IsUpdating = false;
                }

                Data.Archive.Flush();

                GameConfig->CloseArchive();

                SetState(Data.Cancelled ? DownloadInfoState::Cancelled : DownloadInfoState::Finished);
            }
        });
    }

    void DownloadInfo::SetDownloadRunning(bool Running)
    {
        auto& Data = GetStateData<StateInstalling>();

        Data.Pool.SetRunning(Running);
        SetState(Running ? DownloadInfoState::Installing : DownloadInfoState::Paused);
    }

    void DownloadInfo::SetDownloadCancelled()
    {
        auto& Data = GetStateData<StateInstalling>();

        {
            std::unique_lock Lock(Data.DataMutex);
            Data.Cancelled = true;
        }
        SetState(DownloadInfoState::Cancelling);

        Data.Pool.SetRunning(); // They need to run in order to exit
    }

    template<class T>
    T Pop(std::vector<T>& Data) {
        T Ret = Data.back();
        Data.pop_back();
        return Ret;
    }

    bool DownloadInfo::InstallOne()
    {
        auto& Data = GetStateData<StateInstalling>();

        std::unique_lock Lock(Data.DataMutex);

        if (Data.Cancelled) {
            return false;
        }

        if (!Data.UpdatedChunks.empty()) {
            auto& Chunk = Pop(Data.UpdatedChunks).get();
            if (!Data.DeletedChunkIdxs.empty()) {
                uint32_t ReplaceIdx = Pop(Data.DeletedChunkIdxs);
                Lock.unlock();

                auto& ChunkInfoData = Data.ArchiveChunkInfos[ReplaceIdx];

                // Original chunk doesn't have enough space for the new chunk data
                if (Chunk.WindowSize > ChunkInfoData.UncompressedSize) {
                    ChunkInfoData.DataSector = AllocateChunkData(Chunk.WindowSize);
                }

                Data.BytesReadTotal.fetch_add(sizeof(ChunkInfoData), std::memory_order::relaxed);

                InstallOne(Chunk, ChunkInfoData);
                ++Data.PiecesComplete;
                return true;
            }
            else {
                Lock.unlock();

                auto& ChunkInfoData = AllocateChunkInfo();

                ChunkInfoData.DataSector = AllocateChunkData(Chunk.WindowSize);

                InstallOne(Chunk, ChunkInfoData);
                ++Data.PiecesComplete;
                return true;
            }
        }
        else {
            // remove unused deleted chunks later, that's too much work atm
            // for now, we only replace them
            auto Size = Data.DeletedChunkIdxs.size();
            EGL3_ENSUREF(Size == 0, LogLevel::Warning, "{} undeleted chunks remain. This is untested, in the worst case, your install could become corrupted", Size);
            return false;
        }
    }

    void DownloadInfo::InstallOne(const Web::Epic::BPS::ChunkInfo& Chunk, Storage::Game::ChunkInfo& ChunkInfoData)
    {
        auto& Data = GetStateData<StateInstalling>();

        std::unique_ptr<char[]> ChunkData;

        if (Data.EGLProvider.IsValid()) {
            ChunkData = Data.EGLProvider.GetChunk(Chunk.Guid);
            Data.BytesReadTotal.fetch_add((uint64_t)Chunk.WindowSize, std::memory_order::relaxed);
        }

        if (!ChunkData) {
            auto Resp = GetChunk(Chunk);
            EGL3_VERIFY(!Resp.HasError(), "Could not get chunk");
            EGL3_VERIFY(!Resp->HasError(), "Could not parse chunk");

            Data.BytesDownloadTotal.fetch_add((uint64_t)Resp->Header.HeaderSize + Resp->Header.DataSizeCompressed, std::memory_order::relaxed);

            EGL3_ENSURE(Resp->Header.DataSizeUncompressed == Chunk.WindowSize, LogLevel::Error, "Decompressed size is not equal to window size");

            ChunkData = std::move(Resp->Data);
        }

        ChunkInfoData.Guid = Chunk.Guid;
        memcpy(ChunkInfoData.SHA, Chunk.SHAHash, 20);
        ChunkInfoData.UncompressedSize = Chunk.WindowSize;
        ChunkInfoData.CompressedSize = Chunk.WindowSize;
        // ChunkInfoData.DataSector
        ChunkInfoData.Hash = Chunk.Hash;
        ChunkInfoData.DataGroup = Chunk.GroupNumber;

        Data.BytesWriteTotal.fetch_add(sizeof(ChunkInfoData), std::memory_order::relaxed);

        auto BeginChunkDataItr = Data.ArchiveChunkDatas.begin() + ChunkInfoData.DataSector * Game::Header::GetSectorSize();
        BeginChunkDataItr.FastWrite(ChunkData.get(), Chunk.WindowSize);
        // Data.ArchiveChunkDatas.flush(BeginChunkDataItr, Chunk.WindowSize);

        Data.BytesWriteTotal.fetch_add(Chunk.WindowSize, std::memory_order::relaxed);
    }

    Web::Response<Web::Epic::BPS::ChunkData> DownloadInfo::GetChunk(const Web::Epic::BPS::ChunkInfo& Chunk) const
    {
        auto& Data = GetStateData<StateInstalling>();

        Web::Response<Web::Epic::BPS::ChunkData> Resp;
        for (int i = 0; i < 5; ++i) { // Max 5 retries
            Resp = Web::Epic::EpicClient().GetChunk(Data.CloudDir, Data.Manifest.ManifestMeta.FeatureLevel, Chunk);
            if (Resp.HasError()) {
                continue;
            }
            if (Resp->HasError()) {
                continue;
            }
            break;
        }

        return Resp;
    }

    Storage::Game::ChunkInfo& DownloadInfo::AllocateChunkInfo()
    {
        auto& Data = GetStateData<StateInstalling>();

        std::lock_guard Guard(Data.ChunkInfoMutex);

        return Data.ArchiveChunkInfos.emplace_back();
    }

    uint32_t DownloadInfo::AllocateChunkData(uint32_t WindowSize)
    {
        auto& Data = GetStateData<StateInstalling>();

        std::lock_guard Guard(Data.ChunkDataMutex);

        auto StartOffset = Utils::Align<Game::Header::GetSectorSize()>(Data.ArchiveChunkDatas.size());
        auto EndOffset = Utils::Align<Game::Header::GetSectorSize()>(StartOffset + WindowSize);

        Data.ArchiveChunkDatas.resize(EndOffset);

        return StartOffset / Game::Header::GetSectorSize();
    }

    int64_t GetNumberFromMatch(const std::ssub_match& Match) {
        auto Val = Match.str();
        int64_t Ret = 0;
        std::from_chars(Val.c_str(), Val.c_str() + Val.size(), Ret);
        return Ret;
    }

    void DownloadInfo::InitializeArchive(Game::GameId Id, const Web::Epic::BPS::ManifestMeta& Meta, const std::string& CloudDir, Game::ArchiveRef<Game::Header> Header, Game::ArchiveRef<Game::ManifestData> ManifestData)
    {
        Header->SetGameId(Id);
        Header->GetUpdateInfo().IsUpdating = true;
        Header->GetUpdateInfo().NanosecondsElapsed = 0;
        Header->GetUpdateInfo().PiecesComplete = 0;
        Header->GetUpdateInfo().BytesDownloadTotal = 0;

        std::string GameName;
        EGL3_VERIFY(Modules::Game::GameInfoModule::GetGameName(Id, GameName), "Could not get game name");

        uint64_t VersionNum;
        std::string VersionHR;
        Modules::Game::GameInfoModule::ParseGameVersion(Id, Meta.BuildVersion, VersionNum, VersionHR);

        Header->SetGame(GameName);
        Header->SetVersionLong(Meta.BuildVersion);
        Header->SetVersionHR(VersionHR);
        Header->SetVersionNum(VersionNum);
        Header->GetUpdateInfo().TargetVersion = VersionNum;

        ManifestData->SetAppID(Meta.AppId);
        ManifestData->SetLaunchExe(Meta.LaunchExe);
        ManifestData->SetLaunchCommand(Meta.LaunchCommand);
        ManifestData->SetAppName(Meta.AppName);
        ManifestData->SetCloudDir(CloudDir);
    }
}