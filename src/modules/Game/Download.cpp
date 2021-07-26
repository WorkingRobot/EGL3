#include "Download.h"

#include "../../utils/Humanize.h"

namespace EGL3::Modules::Game {
    using namespace Storage::Models;

    static constexpr double GraphScale = 1024. * 1024 * 50;
    static constexpr std::chrono::milliseconds RefreshTime(500);
    static constexpr double DivideRate = std::chrono::duration_cast<std::chrono::duration<double>>(RefreshTime) / std::chrono::seconds(1);

    namespace Colors {
        static constexpr Widgets::StateColor Unknown = 0xFFC3C2C2;
        static constexpr Widgets::StateColor PendingLocalChunkDbData = 0xFFA390CB;
        static constexpr Widgets::StateColor RetrievingLocalChunkDbData = 0xFFB1A6D6;
        static constexpr Widgets::StateColor PendingLocalInstallData = 0xFFA390CB;
        static constexpr Widgets::StateColor RetrievingLocalInstallData = 0xFFB1A6D6;
        static constexpr Widgets::StateColor PendingRemoteCloudData = 0xFF90B1CB;
        static constexpr Widgets::StateColor RetrievingRemoteCloudData = 0xFFA6C6D6;
        static constexpr Widgets::StateColor PendingLocalDataStore = 0xFFA390CB;
        static constexpr Widgets::StateColor RetrievingLocalDataStore = 0xFFB1A6D6;
        static constexpr Widgets::StateColor DataInMemoryStore = 0xFFC3C2C2;
        static constexpr Widgets::StateColor Staged = 0xFFC8E2BF;
        static constexpr Widgets::StateColor Installed = 0xFFC8E2BF;
        static constexpr Widgets::StateColor Verifying = 0xFFAFD6A6;
        static constexpr Widgets::StateColor VerifiedFail = 0xFFD62728;
        static constexpr Widgets::StateColor VerifiedSuccess = 0xFF95CB90;
    }

    static constexpr Widgets::StateColor ChunkStateToColor(ChunkState State) {
        switch (State)
        {
        case ChunkState::Scheduled:
            return Colors::PendingRemoteCloudData;
        case ChunkState::Initializing:
            return Colors::Unknown;
        case ChunkState::Transferring:
            return Colors::RetrievingLocalInstallData;
        case ChunkState::Downloading:
            return Colors::RetrievingRemoteCloudData;
        case ChunkState::WritingMetadata:
            return Colors::DataInMemoryStore;
        case ChunkState::WritingData:
            return Colors::Installed;
        case ChunkState::Completed:
            return Colors::VerifiedSuccess;
        default:
            return Colors::Unknown;
        }
    }

    DownloadModule::DownloadModule(ModuleList& Ctx) :
        Auth(Ctx.GetModule<Login::AuthModule>()),
        GameInfo(Ctx.GetModule<GameInfoModule>()),
        Taskbar(Ctx.GetModule<TaskbarModule>()),
        MainStack(Ctx.GetWidget<Gtk::Stack>("MainStack")),
        SwitchStack(Ctx.GetWidget<Gtk::Stack>("DownloadStack")),
        SwitchStackPageOptions(Ctx.GetWidget<Gtk::Box>("DownloadStackPage0")),
        OptionsBrowseBtn(Ctx.GetWidget<Gtk::Button>("DownloadOptionsFileChooser")),
        // vvv The main window is used in the constructor for the dialog, don't confuse this with the actual main window vvv
        OptionsFileDialog(Ctx.GetWidget<Gtk::Window>("EGL3App")),
        OptionsFilePreview(Ctx.GetWidget<Gtk::Label>("DownloadOptionsFilePreview")),
        OptionsAutoUpdate(Ctx.GetWidget<Gtk::CheckButton>("DownloadOptionsAutoUpdate")),
        OptionsCreateShortcut(Ctx.GetWidget<Gtk::CheckButton>("DownloadOptionsCreateShortcut")),
        OptionsSdMeta(Ctx.GetWidget<Gtk::TreeView>("DownloadOptionsSelector")),
        SwitchStackPageInfo(Ctx.GetWidget<Gtk::ScrolledWindow>("DownloadStackPage1")),
        InfoButtonPause(Ctx.GetWidget<Gtk::Button>("DownloadInfoPauseBtn")),
        InfoButtonStop(Ctx.GetWidget<Gtk::Button>("DownloadInfoStopBtn")),
        InfoButtonPauseImage(Ctx.GetWidget<Gtk::Image>("DownloadInfoPauseImg")),
        InfoState(Ctx.GetWidget<Gtk::Label>("DownloadInfoState")),
        InfoPercent(Ctx.GetWidget<Gtk::Label>("DownloadInfoPercent")),
        InfoProgressBar(Ctx.GetWidget<Gtk::ProgressBar>("DownloadInfoProgress")),
        InfoTimeElapsed(Ctx.GetWidget<Gtk::Label>("DownloadInfoTimeElapsed")),
        InfoTimeRemaining(Ctx.GetWidget<Gtk::Label>("DownloadInfoTimeRemaining")),
        InfoPieces(Ctx.GetWidget<Gtk::Label>("DownloadInfoPieces")),
        InfoDownloaded(Ctx.GetWidget<Gtk::Label>("DownloadInfoDownloaded")),
        InfoNetworkCurrent(Ctx.GetWidget<Gtk::Label>("DownloadInfoNetworkCurrent")),
        InfoNetworkPeak(Ctx.GetWidget<Gtk::Label>("DownloadInfoNetworkPeak")),
        InfoReadCurrent(Ctx.GetWidget<Gtk::Label>("DownloadInfoReadCurrent")),
        InfoReadPeak(Ctx.GetWidget<Gtk::Label>("DownloadInfoReadPeak")),
        InfoWriteCurrent(Ctx.GetWidget<Gtk::Label>("DownloadInfoWriteCurrent")),
        InfoWritePeak(Ctx.GetWidget<Gtk::Label>("DownloadInfoWritePeak")),
        InfoStateGrid(Ctx.GetWidget<Gtk::DrawingArea>("DownloadInfoGraph"), ChunkStateToColor)
    {
        SlotBrowse = OptionsBrowseBtn.signal_clicked().connect([this]() { OptionsFileDialog.Show(); });

        OptionsFileDialog.LocationChosen.Set([this](const std::string& NewFile) {
            auto& Data = CurrentDownload->GetStateData<DownloadInfo::StateOptions>();
            if (NewFile.empty()) {
                OptionsFilePreview.set_text(Data.DefaultArchivePath.string().c_str());
                OptionsFilePreview.set_tooltip_text(Data.DefaultArchivePath.string().c_str());

                Data.ArchivePath = Data.DefaultArchivePath;
            }
            else {
                OptionsFilePreview.set_text(NewFile);
                OptionsFilePreview.set_tooltip_text(NewFile);

                Data.ArchivePath = NewFile;
            }
        });

        SlotPause = InfoButtonPause.signal_clicked().connect([this]() { OnDownloadPauseClicked(); });
        SlotStop = InfoButtonStop.signal_clicked().connect([this]() { OnDownloadStopClicked(); });

        StatsDispatcher.connect([this](const Storage::Models::DownloadInfoStats& Stats) { OnStatsUpdate(Stats); });
        StateDispatcher.connect([this](Storage::Models::DownloadInfoState State) { OnStateUpdate(State); });

        MainStackBefore = MainStackCurrent = MainStack.get_focus_child();

        SlotSwitchedFocus = MainStack.signal_set_focus_child().connect([this](Gtk::Widget* NewChild) {
            MainStackBefore = MainStackCurrent;
            MainStackCurrent = NewChild;
            if (MainStackBefore == &SwitchStack) {
                if (CurrentDownload) {
                    switch (CurrentDownload->GetState())
                    {
                    case DownloadInfoState::Cancelled:
                    case DownloadInfoState::Finished:
                        SwitchStack.hide();
                        CurrentDownload.release();
                        break;
                    default:
                        break;
                    }
                }
                else {
                    SwitchStack.hide();
                }
            }
        });

        Utils::Mmio::MmioFile::SetWorkingSize(384ull * 1024 * 1024);

        SlotLogOutPreflight = Auth.LogOutPreflight.connect([this, &Ctx]() {
            if (CurrentDownload) {
                if (CurrentDownload->GetState() == DownloadInfoState::Options ||
                    CurrentDownload->GetState() == DownloadInfoState::Finished ||
                    CurrentDownload->GetState() == DownloadInfoState::Cancelled) {
                    return true;
                }

                if (!Ctx.DisplayConfirmation("A download is currently active. Are you sure you want to quit?" "<i>You can resume it at a later time.</i>", "Download Currently Active", true)) {
                    return false;
                }

                CurrentDownload->SetDownloadCancelled();
            }
            return true;
        });
    }

    DownloadModule::~DownloadModule()
    {
        if (CurrentDownload) {
            CurrentDownload->OnStateUpdate.clear();
            CurrentDownload->OnStatsUpdate.Set([](const auto& Stats) {});
        }
    }

    DownloadInfo& DownloadModule::OnDownloadClicked(Storage::Game::GameId Id, InstalledGame* GameConfig)
    {
        ResetStats();

        CurrentDownload = std::make_unique<DownloadInfo>(Id, GameConfig);

        CurrentDownload->OnStateUpdate.connect([this](DownloadInfoState NewState) {
            if (NewState == DownloadInfoState::Initializing && OptionsIsUsingEGL.has_value()) {
                Glib::signal_idle().connect_once([this]() {
                    SwitchStackPageOptions.remove(OptionsIsUsingEGL.value());
                    OptionsIsUsingEGL.reset();
                });
            }
            if (NewState == DownloadInfoState::Installing) {
                auto& Data = CurrentDownload->GetStateData<DownloadInfo::StateInstalling>();
                InfoStateGrid.Initialize(Data.UpdatedChunks.size());
            }

            StateDispatcher.emit(NewState);
        });

        CurrentDownload->OnStatsUpdate.Set([this](const DownloadInfoStats& NewStats) {
            StatsDispatcher.emit(NewStats);
        });

        CurrentDownload->OnChunkUpdate.Set([this](const Utils::Guid& Guid, ChunkState NewState) {
            switch (NewState)
            {
            case ChunkState::Initializing:
                InfoStateGrid.BeginCtx(Guid, NewState);
                break;
            case ChunkState::Completed:
                InfoStateGrid.EndCtx(Guid);
                break;
            default:
                InfoStateGrid.UpdateCtx(Guid, NewState);
                break;
            }
        });

        auto VersionData = GameInfo.GetVersionData(Id);
        EGL3_VERIFY(VersionData, "Version data should be valid at this point");
        auto InstallOpts = GameInfo.GetInstallOptions(Id, VersionData->Element.BuildVersion);

        auto& Data = CurrentDownload->GetStateData<DownloadInfo::StateOptions>();
        OptionsFileDialog.SetLocation(Data.ArchivePath.string());
        OptionsAutoUpdate.set_active(GetInstallFlag<InstallFlags::AutoUpdate>(Data.Flags));
        OptionsCreateShortcut.set_active(GetInstallFlag<InstallFlags::CreateShortcut>(Data.Flags));
        OptionsSdMeta.Initialize(InstallOpts);
        if (GetInstallFlag<InstallFlags::DefaultSelectedIds>(Data.Flags)) {
            OptionsSdMeta.SetDefaultOptions();
        }
        else if (GetInstallFlag<InstallFlags::SelectedIds>(Data.Flags)) {
            OptionsSdMeta.SetOptions(Data.SelectedIds);
        }
        else {
            OptionsSdMeta.SetAllOptions();
        }
        if (Data.EGLProvider.IsValid()) {
            auto& Label = OptionsIsUsingEGL.emplace();
            Label.set_markup(std::format("An existing install was found at <b>{}</b>.\nAny compatible data found there will be used, otherwise the remaining data will be redownloaded.", (std::string)Glib::Markup::escape_text(Data.EGLProvider.GetInstallLocation().string())));
            Label.set_justify(Gtk::JUSTIFY_CENTER);
            SwitchStackPageOptions.pack_end(Label, false, false);
            Label.show();
        }

        SwitchStack.set_visible_child(SwitchStackPageOptions);
        SwitchStack.show();
        MainStack.set_visible_child(SwitchStack);

        return *CurrentDownload;
    }

    void DownloadModule::OnDownloadOkClicked(const DownloadInfo::CreateGameConfig& CreateGameConfig)
    {
        auto& Data = CurrentDownload->GetStateData<DownloadInfo::StateOptions>();
        Data.Flags = SetInstallFlag<InstallFlags::AutoUpdate>(InstallFlags::SelectedIds, OptionsAutoUpdate.get_active());
        Data.Flags = SetInstallFlag<InstallFlags::CreateShortcut>(Data.Flags, OptionsCreateShortcut.get_active());
        OptionsSdMeta.GetOptions(Data.SelectedIds, Data.InstallTags);

        CurrentDownload->BeginDownload(
            [this](Storage::Game::GameId Id, std::string& CloudDir) -> Web::Response<Web::Epic::BPS::Manifest> {
                auto Resp = GameInfo.GetVersionData(Id);
                if (!Resp) {
                    return Web::ErrorData::Status::Failure;
                }

                return Web::Epic::EpicClient().GetManifest(Resp->Element, CloudDir);
            },
            CreateGameConfig
        );
        SwitchStack.set_visible_child(SwitchStackPageInfo);
    }

    void DownloadModule::OnDownloadCancelClicked()
    {
        CurrentDownload->CancelDownloadSetup();
        SwitchStack.hide();
    }

    void DownloadModule::OnDownloadPauseClicked()
    {
        InfoButtonPause.set_sensitive(false);
        InfoButtonStop.set_sensitive(false);

        CurrentDownload->SetDownloadRunning(CurrentDownload->GetState() == DownloadInfoState::Paused);
    }

    void DownloadModule::OnDownloadStopClicked()
    {
        InfoButtonPause.set_sensitive(false);
        InfoButtonStop.set_sensitive(false);

        CurrentDownload->SetDownloadCancelled();
    }

    void DownloadModule::ResetStats()
    {
        PiecesCompletedLast = 0;
        StatsBytesDownloadPeak = 0;
        StatsBytesReadPeak = 0;
        StatsBytesWritePeak = 0;

        InfoButtonPause.set_sensitive(false);
        InfoButtonStop.set_sensitive(false);
        InfoButtonPauseImage.set_from_icon_name("media-playback-pause-symbolic", Gtk::ICON_SIZE_BUTTON);
        InfoState.set_text("");
        InfoPercent.set_text("");
        InfoProgressBar.set_fraction(0);
        InfoTimeElapsed.set_text("");
        InfoTimeRemaining.set_text("");
        InfoPieces.set_text("");
        InfoDownloaded.set_text("");
        InfoNetworkCurrent.set_text("");
        InfoNetworkPeak.set_text("");
        InfoReadCurrent.set_text("");
        InfoReadPeak.set_text("");
        InfoWriteCurrent.set_text("");
        InfoWritePeak.set_text("");
        InfoStateGrid.Initialize(0);
    }

    template<>
    std::chrono::nanoseconds DownloadModule::CalculateEndTimestamp<0>(uint32_t AmountLeft)
    {
        std::vector<double> RateAvg;
        RateAvg.reserve(StatsRateHistory.size());

        for (int n = 0; n < StatsRateHistory.size(); ++n) {
            if (n == 0) {
                RateAvg.emplace_back(StatsRateHistory[n]);
            }
            else {
                auto Weight = 1. / (n + 1);

                RateAvg.emplace_back(StatsRateHistory[n] * Weight + RateAvg[n - 1] * (1 - Weight));
            }
        }

        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(AmountLeft / RateAvg.back()));
    }

    template<>
    std::chrono::nanoseconds DownloadModule::CalculateEndTimestamp<1>(uint32_t AmountLeft)
    {
        std::vector<double> RateAvg;
        RateAvg.reserve(StatsRateHistory.size());

        for (int n = 0; n < StatsRateHistory.size(); ++n) {
            if (n == 0) {
                RateAvg.emplace_back(StatsRateHistory[n]);
            }
            else if (n == 1) {
                RateAvg.emplace_back((StatsRateHistory[n] + StatsRateHistory[n - 1]) / 2.);
            }
            else {
                auto Weight = 1. / n;

                RateAvg.emplace_back(StatsRateHistory[n] * Weight + RateAvg[n - 1] * (1 - Weight));
            }
        }

        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(AmountLeft / RateAvg.back()));
    }

    template<>
    std::chrono::nanoseconds DownloadModule::CalculateEndTimestamp<2>(uint32_t AmountLeft)
    {
        std::vector<double> RateAvg;
        RateAvg.reserve(StatsRateHistory.size());

        for (int n = 0; n < StatsRateHistory.size(); ++n) {
            if (n == 0) {
                RateAvg.emplace_back(StatsRateHistory[n]);
            }
            else {
                auto Weight = 1. / 2;

                RateAvg.emplace_back(StatsRateHistory[n] * Weight + RateAvg[n - 1] * (1 - Weight));
            }
        }

        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(AmountLeft / RateAvg.back()));
    }

    void DownloadModule::OnStatsUpdate(const Storage::Models::DownloadInfoStats& Stats)
    {
        StatsRateHistory.emplace_back(PiecesCompletedLast ? (Stats.PiecesComplete - PiecesCompletedLast) / DivideRate : 0);
        PiecesCompletedLast = Stats.PiecesComplete;
        while (StatsRateHistory.size() > 50) {
            StatsRateHistory.pop_front();
        }

        std::chrono::steady_clock::time_point BeginTimestamp(std::chrono::nanoseconds(Stats.NanosecondStartTimestamp));
        std::chrono::steady_clock::time_point CurrentTimestamp(std::chrono::nanoseconds(Stats.NanosecondCurrentTimestamp));
        auto EndTimestamp = CurrentTimestamp + CalculateEndTimestamp<1>(Stats.PiecesTotal - Stats.PiecesComplete);

        auto PiecesTotalCorrected = std::max(Stats.PiecesTotal, 1u);
        double CompletionPercent = (double)Stats.PiecesComplete / PiecesTotalCorrected;

        Taskbar.SetProgressValue(Stats.PiecesComplete, PiecesTotalCorrected);

        InfoState.set_text(DownloadInfoStateToString(Stats.State));

        InfoPercent.set_text(std::format("{:.0f}%", CompletionPercent * 100));
        InfoProgressBar.set_fraction(CompletionPercent);

        InfoTimeElapsed.set_text(Utils::Humanize(BeginTimestamp, CurrentTimestamp));
        InfoTimeElapsed.set_tooltip_text(Glib::ustring::format(Utils::HumanizeExact(CurrentTimestamp - BeginTimestamp)));
            
        InfoTimeRemaining.set_text(Utils::Humanize(EndTimestamp, CurrentTimestamp));
        InfoTimeRemaining.set_tooltip_text(Glib::ustring::format(Utils::HumanizeExact(EndTimestamp - CurrentTimestamp)));

        InfoPieces.set_text(Glib::ustring::compose("%1/%2", Utils::Humanize(Stats.PiecesComplete), Utils::Humanize(Stats.PiecesTotal)));
        InfoDownloaded.set_text(Glib::ustring::compose("%1/%2", Utils::HumanizeByteSize(Stats.BytesDownloadTotal), Utils::HumanizeByteSize(Stats.DownloadTotal)));
            
        if (StatsBytesDownloadPeak < Stats.BytesDownloadRate) {
            StatsBytesDownloadPeak = Stats.BytesDownloadRate;
        }
        InfoNetworkCurrent.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(Stats.BytesDownloadRate)));
        InfoNetworkCurrent.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(Stats.BytesDownloadRate)));
        InfoNetworkPeak.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsBytesDownloadPeak)));
        InfoNetworkPeak.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsBytesDownloadPeak)));

        if (StatsBytesReadPeak < Stats.BytesReadRate) {
            StatsBytesReadPeak = Stats.BytesReadRate;
        }
        InfoReadCurrent.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(Stats.BytesReadRate)));
        InfoReadCurrent.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(Stats.BytesReadRate)));
        InfoReadPeak.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsBytesReadPeak)));
        InfoReadPeak.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsBytesReadPeak)));

        if (StatsBytesWritePeak < Stats.BytesWriteRate) {
            StatsBytesWritePeak = Stats.BytesWriteRate;
        }
        InfoWriteCurrent.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(Stats.BytesWriteRate)));
        InfoWriteCurrent.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(Stats.BytesWriteRate)));
        InfoWritePeak.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsBytesWritePeak)));
        InfoWritePeak.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsBytesWritePeak)));
    }

    void DownloadModule::OnStateUpdate(Storage::Models::DownloadInfoState State)
    {
        InfoState.set_text(DownloadInfoStateToString(State));
        switch (State)
        {
        case DownloadInfoState::Installing:
            Taskbar.SetProgressState(Utils::Taskbar::ProgressState::Normal);
            break;
        case DownloadInfoState::Paused:
            Taskbar.SetProgressState(Utils::Taskbar::ProgressState::Paused);
            break;
        case DownloadInfoState::Cancelling:
        case DownloadInfoState::Cancelled:
            Taskbar.SetProgressState(Utils::Taskbar::ProgressState::Error);
            break;
        case DownloadInfoState::Initializing:
            Taskbar.SetProgressState(Utils::Taskbar::ProgressState::Indeterminate);
            break;
        default:
            Taskbar.SetProgressState(Utils::Taskbar::ProgressState::None);
            break;
        }

        bool IsOnInstallStep = State == DownloadInfoState::Installing || State == DownloadInfoState::Paused;
        InfoButtonPause.set_sensitive(IsOnInstallStep);
        InfoButtonStop.set_sensitive(IsOnInstallStep);

        if (IsOnInstallStep) {
            InfoButtonPauseImage.set_from_icon_name(State == DownloadInfoState::Installing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic", Gtk::ICON_SIZE_BUTTON);
        }
    }
}