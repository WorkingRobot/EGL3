#pragma once

#include "../Login/Auth.h" // Weird rapidjson vs windows schemery, this is here (and not lower) for a reason
#include "../../storage/models/DownloadInfo.h"
#include "../../utils/Callback.h"
#include "../../utils/SlotHolder.h"
#include "../../widgets/InstallLocationDialog.h"
#include "../../widgets/SdTree.h"
#include "../../widgets/StateGrid.h"
#include "../ModuleList.h"
#include "../Taskbar.h"
#include "GameInfo.h"

#include <chrono>
#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class DownloadModule : public BaseModule {
    public:
        DownloadModule(ModuleList& Ctx);

        ~DownloadModule();

        Storage::Models::DownloadInfo& OnDownloadClicked(Storage::Game::GameId Id);

        void OnDownloadOkClicked();

        void OnDownloadCancelClicked();

        void OnDownloadPauseClicked();

        void OnDownloadStopClicked();

    private:
        void ResetStats();

        // Called by dispatcher
        void OnStatsUpdate();

        // StatsMutex should be locked by the thread when calling
        template<int Type>
        std::chrono::nanoseconds CalculateEndTimestamp(uint32_t AmountLeft);

        Storage::Persistent::Store& Storage;
        Login::AuthModule& Auth;
        GameInfoModule& GameInfo;
        TaskbarModule& Taskbar;

        Gtk::Stack& MainStack;
        Gtk::Stack& SwitchStack;

        Gtk::Box& SwitchStackPageOptions;
        Gtk::Button& OptionsBrowseBtn;
        Widgets::InstallLocationDialog OptionsFileDialog;
        Gtk::Label& OptionsFilePreview;
        Gtk::CheckButton& OptionsAutoUpdate;
        Gtk::CheckButton& OptionsCreateShortcut;
        Widgets::SdTree OptionsSdMeta;
        std::optional<Gtk::Label> OptionsIsUsingEGL;

        Gtk::ScrolledWindow& SwitchStackPageInfo;
        Gtk::Button& InfoButtonPause;
        Gtk::Button& InfoButtonStop;
        Gtk::Image& InfoButtonPauseImage;
        Gtk::Label& InfoState;
        Gtk::Label& InfoPercent;
        Gtk::ProgressBar& InfoProgressBar;
        Gtk::Label& InfoTimeElapsed;
        Gtk::Label& InfoTimeRemaining;
        Gtk::Label& InfoPieces;
        Gtk::Label& InfoDownloaded;
        Gtk::Label& InfoNetworkCurrent;
        Gtk::Label& InfoNetworkPeak;
        Gtk::Label& InfoReadCurrent;
        Gtk::Label& InfoReadPeak;
        Gtk::Label& InfoWriteCurrent;
        Gtk::Label& InfoWritePeak;
        Widgets::StateGrid<10, Utils::Guid, Storage::Models::ChunkState, Storage::Models::ChunkState::Scheduled, Storage::Models::ChunkState::Completed> InfoStateGrid;

        Utils::SlotHolder SlotBrowse;
        Utils::SlotHolder SlotPause;
        Utils::SlotHolder SlotStop;
        Utils::SlotHolder SlotSwitchedFocus;

        Gtk::Widget* MainStackBefore;
        Gtk::Widget* MainStackCurrent;

        std::unique_ptr<Storage::Models::DownloadInfo> CurrentDownload;

        std::mutex StatsMutex;
        Storage::Models::DownloadInfoStats StatsData;

        std::deque<double> StatsRateHistory; // Used when calculating ETA
        uint32_t PiecesCompletedLast;
        uint64_t StatsBytesDownloadPeak;
        uint64_t StatsBytesReadPeak;
        uint64_t StatsBytesWritePeak;

        Glib::Dispatcher StatsDispatcher;

        std::vector<Web::Epic::Content::SdMeta::Data> InstallOpts;
    };
}