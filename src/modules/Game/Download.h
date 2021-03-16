#pragma once

#include "../../storage/persistent/Store.h"
#include "../Authorization.h" // Weird rapidjson vs windows schemery, this is here (and not lower) for a reason
#include "../../storage/models/DownloadInfo.h"
#include "../../utils/Callback.h"
#include "../../utils/GladeBuilder.h"
#include "../../widgets/Graph.h"
#include "../../widgets/InstallLocationDialog.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../Taskbar.h"
#include "GameInfo.h"

#include <chrono>
#include <gtkmm.h>

namespace EGL3::Modules::Game {
    class DownloadModule : public BaseModule {
    public:
        DownloadModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

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
        Modules::AuthorizationModule& Auth;
        GameInfoModule& GameInfo;
        Modules::TaskbarModule& Taskbar;

        Gtk::Stack& MainStack;
        Gtk::Stack& SwitchStack;

        Gtk::Box& SwitchStackPageOptions;
        Gtk::Button& OptionsBrowseBtn;
        Widgets::InstallLocationDialog OptionsFileDialog;
        Gtk::Label& OptionsFilePreview;
        Gtk::CheckButton& OptionsAutoUpdate;
        Gtk::CheckButton& OptionsCreateShortcut;
        Gtk::Button& OptionsButtonOk;
        Gtk::Button& OptionsButtonCancel;

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
        Widgets::Graph<3> InfoGraph;

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
    };
}