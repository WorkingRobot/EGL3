#include "Download.h"

#include "../../utils/Humanize.h"

namespace EGL3::Modules::Game {
    using namespace Storage::Models;

    static constexpr double GraphScale = 1024. * 1024 * 50;
    static constexpr std::chrono::milliseconds RefreshTime(500);
    static constexpr double DivideRate = std::chrono::duration_cast<std::chrono::duration<double>>(RefreshTime) / std::chrono::seconds(1);

    DownloadModule::DownloadModule(ModuleList& Ctx) :
        Storage(Ctx.GetStorage()),
        Auth(Ctx.GetModule<AuthorizationModule>()),
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
        OptionsButtonOk(Ctx.GetWidget<Gtk::Button>("DownloadOptionsOk")),
        OptionsButtonCancel(Ctx.GetWidget<Gtk::Button>("DownloadOptionsCancel")),
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
        InfoGraph(Ctx.GetWidget<Gtk::DrawingArea>("DownloadInfoGraph"))
    {
        OptionsBrowseBtn.signal_clicked().connect([this]() {
            OptionsFileDialog.Show();
        });

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

        OptionsButtonOk.signal_clicked().connect([this]() { OnDownloadOkClicked(); });
        OptionsButtonCancel.signal_clicked().connect([this]() { OnDownloadCancelClicked(); });

        InfoButtonPause.signal_clicked().connect([this]() { OnDownloadPauseClicked(); });
        InfoButtonStop.signal_clicked().connect([this]() { OnDownloadStopClicked(); });

        StatsDispatcher.connect([this]() { OnStatsUpdate(); });

        InfoGraph.OnFormatTooltip.Set([this](int Idx, const auto& Data) {
            return Glib::ustring::compose("Network: %1/s\nRead: %2/s\nWrite: %3/s",
                Utils::HumanizeByteSize(Data[0] * GraphScale),
                Utils::HumanizeByteSize(Data[1] * GraphScale),
                Utils::HumanizeByteSize(Data[2] * GraphScale)
            );
        });

        MainStackBefore = MainStackCurrent = MainStack.get_focus_child();

        MainStack.signal_set_focus_child().connect([this](Gtk::Widget* NewChild) {
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

        Utils::Mmio::MmioFile::SetWorkingSize(Utils::Mmio::MmioFile::DownloadWorkingSize);
    }

    DownloadInfo& DownloadModule::OnDownloadClicked(Storage::Game::GameId Id)
    {
        ResetStats();

        CurrentDownload = std::make_unique<DownloadInfo>(Id, [this]() -> std::vector<InstalledGame>& { return Storage.Get(Storage::Persistent::Key::InstalledGames); });

        CurrentDownload->OnStateUpdate.connect([this](DownloadInfoState NewState) {
            {
                std::lock_guard Lock(StatsMutex);

                StatsData.State = NewState;
                StatsData.PiecesTotal = -1;
            }
            StatsDispatcher.emit();
        });

        CurrentDownload->OnStatsUpdate.Set([this](const DownloadInfoStats& NewStats) {
            {
                std::lock_guard Lock(StatsMutex);

                StatsData = NewStats;
            }
            StatsDispatcher.emit();
        });

        auto VersionData = GameInfo.GetVersionData(Id);
        EGL3_CONDITIONAL_LOG(VersionData, LogLevel::Critical, "Version data should be valid at this point");
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

        SwitchStack.set_visible_child(SwitchStackPageOptions);
        SwitchStack.show();
        MainStack.set_visible_child(SwitchStack);

        return *CurrentDownload;
    }

    void DownloadModule::OnDownloadOkClicked()
    {
        auto& Data = CurrentDownload->GetStateData<DownloadInfo::StateOptions>();
        Data.Flags = SetInstallFlag<InstallFlags::AutoUpdate>(InstallFlags::SelectedIds, OptionsAutoUpdate.get_active());
        Data.Flags = SetInstallFlag<InstallFlags::CreateShortcut>(Data.Flags, OptionsCreateShortcut.get_active());
        OptionsSdMeta.GetOptions(Data.SelectedIds, Data.InstallTags);

        CurrentDownload->BeginDownload([this](Storage::Game::GameId Id, std::string& CloudDir) -> Web::Response<Web::Epic::BPS::Manifest> {
            auto Resp = GameInfo.GetVersionData(Id);
            if (!Resp) {
                return Web::ErrorData::Status::Failure;
            }

            return Web::Epic::EpicClient().GetManifest(Resp->Element, CloudDir);
        });
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
        InfoGraph.Clear();
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

    void DownloadModule::OnStatsUpdate()
    {
        std::lock_guard Guard(StatsMutex);

        if (StatsData.PiecesTotal == -1) {
            InfoState.set_text(DownloadInfoStateToString(StatsData.State));
            switch (StatsData.State)
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
                Storage.Flush();
                Taskbar.SetProgressState(Utils::Taskbar::ProgressState::Indeterminate);
                break;
            default:
                Taskbar.SetProgressState(Utils::Taskbar::ProgressState::None);
                break;
            }

            bool IsOnInstallStep = StatsData.State == DownloadInfoState::Installing || StatsData.State == DownloadInfoState::Paused;
            InfoButtonPause.set_sensitive(IsOnInstallStep);
            InfoButtonStop.set_sensitive(IsOnInstallStep);

            if (IsOnInstallStep) {
                InfoButtonPauseImage.set_from_icon_name(StatsData.State == DownloadInfoState::Installing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic", Gtk::ICON_SIZE_BUTTON);
            }
        }
        else {
            StatsRateHistory.emplace_back(PiecesCompletedLast ? (StatsData.PiecesComplete - PiecesCompletedLast) / DivideRate : 0);
            PiecesCompletedLast = StatsData.PiecesComplete;
            while (StatsRateHistory.size() > 50) {
                StatsRateHistory.pop_front();
            }

            std::chrono::steady_clock::time_point BeginTimestamp(std::chrono::nanoseconds(StatsData.NanosecondStartTimestamp));
            std::chrono::steady_clock::time_point CurrentTimestamp(std::chrono::nanoseconds(StatsData.NanosecondCurrentTimestamp));
            auto EndTimestamp = CurrentTimestamp + CalculateEndTimestamp<1>(StatsData.PiecesTotal - StatsData.PiecesComplete);

            auto PiecesTotalCorrected = std::max(StatsData.PiecesTotal, 1u);
            double CompletionPercent = (double)StatsData.PiecesComplete / PiecesTotalCorrected;

            Taskbar.SetProgressValue(StatsData.PiecesComplete, PiecesTotalCorrected);

            InfoState.set_text(DownloadInfoStateToString(StatsData.State));

            InfoPercent.set_text(Utils::Format("%.f%%", CompletionPercent * 100));
            InfoProgressBar.set_fraction(CompletionPercent);

            InfoTimeElapsed.set_text(Utils::Humanize(BeginTimestamp, CurrentTimestamp));
            InfoTimeElapsed.set_tooltip_text(Glib::ustring::format(Utils::HumanizeExact(CurrentTimestamp - BeginTimestamp)));
            
            InfoTimeRemaining.set_text(Utils::Humanize(EndTimestamp, CurrentTimestamp));
            InfoTimeRemaining.set_tooltip_text(Glib::ustring::format(Utils::HumanizeExact(EndTimestamp - CurrentTimestamp)));

            InfoPieces.set_text(Glib::ustring::compose("%1/%2", Utils::Humanize(StatsData.PiecesComplete), Utils::Humanize(StatsData.PiecesTotal)));
            InfoDownloaded.set_text(Glib::ustring::compose("%1/%2", Utils::HumanizeByteSize(StatsData.BytesDownloadTotal), Utils::HumanizeByteSize(StatsData.DownloadTotal)));
            
            if (StatsBytesDownloadPeak < StatsData.BytesDownloadRate) {
                StatsBytesDownloadPeak = StatsData.BytesDownloadRate;
            }
            InfoNetworkCurrent.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsData.BytesDownloadRate)));
            InfoNetworkCurrent.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsData.BytesDownloadRate)));
            InfoNetworkPeak.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsBytesDownloadPeak)));
            InfoNetworkPeak.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsBytesDownloadPeak)));

            if (StatsBytesReadPeak < StatsData.BytesReadRate) {
                StatsBytesReadPeak = StatsData.BytesReadRate;
            }
            InfoReadCurrent.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsData.BytesReadRate)));
            InfoReadCurrent.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsData.BytesReadRate)));
            InfoReadPeak.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsBytesReadPeak)));
            InfoReadPeak.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsBytesReadPeak)));

            if (StatsBytesWritePeak < StatsData.BytesWriteRate) {
                StatsBytesWritePeak = StatsData.BytesWriteRate;
            }
            InfoWriteCurrent.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsData.BytesWriteRate)));
            InfoWriteCurrent.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsData.BytesWriteRate)));
            InfoWritePeak.set_text(Glib::ustring::compose("%1/s", Utils::HumanizeByteSize(StatsBytesWritePeak)));
            InfoWritePeak.set_tooltip_text(Glib::ustring::compose("%1ps", Utils::HumanizeBitSize(StatsBytesWritePeak)));

            InfoGraph.Add({ float(StatsData.BytesDownloadRate / GraphScale), float(StatsData.BytesReadRate / GraphScale), float(StatsData.BytesWriteRate / GraphScale) });
        }
    }
}