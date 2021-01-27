#include "Game.h"

#include "../../storage/game/Archive.h"
#include "../../utils/Assert.h"
#include "../../web/epic/EpicClient.h"

namespace EGL3::Modules::Game {
    GameModule::GameModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        AsyncFF(Modules.GetModule<AsyncFFModule>()),
        Auth(Modules.GetModule<AuthorizationModule>()),
        Updater(Modules.GetModule<UpdaterModule>()),
        PlayBtn(Builder.GetWidget<Gtk::Button>("PlayBtn")),
        PlayMenuBtn(Builder.GetWidget<Gtk::MenuButton>("PlayDropdown")),
        PlayMenuVerifyOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlayVerifyOpt")),
        PlayMenuModifyOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlayModifyOpt")),
        PlayMenuSignOutOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlaySignOutOpt")),
        CurrentState(State::SignIn),
        InstallDialog(Builder.GetWidget<Gtk::ApplicationWindow>("EGL3App"))
    {
        std::filesystem::path InstallFolder;
        if (GetInstallFolder(InstallFolder)) {
            std::error_code Error;
            for (auto& File : std::filesystem::directory_iterator(InstallFolder, Error)) {
                if (File.is_regular_file(Error)) {
                    Storage::Game::Archive Archive(File.path(), Storage::Game::ArchiveMode::Read);
                    if (Archive.IsValid()) {
                        InstalledGames.emplace_back(std::make_shared<Storage::Game::Archive>(std::move(Archive)));
                    }
                }
            }
        }

        PlayBtn.signal_clicked().connect([this]() { PlayClicked(); });

        InstallDialog.LocationChosen.Set([this](const std::string& File) {
            this->Storage.Get(Storage::Persistent::Key::InstallLocation) = File;

            StartUpdate();

            CurrentState = State::Installing;
            UpdateToCurrentState();
        });

        CurrentStateDispatcher.connect([this]() { UpdateToCurrentState(); });

        Auth.AuthChanged.connect(sigc::mem_fun(*this, &GameModule::OnAuthChanged));

        UpdateToCurrentState();
    }

    void GameModule::OnAuthChanged()
    {
        CurrentState = IsInstalled(Storage::Game::GameId::Fortnite) ? State::Play : State::Install;
        CurrentStateDispatcher.emit();
    }

    void GameModule::UpdateToState(const char* NewLabel, bool Playable, bool Menuable)
    {
        PlayBtn.set_label(NewLabel);
        PlayBtn.set_sensitive(Playable);
        PlayMenuBtn.set_sensitive(Menuable);
    }

    void GameModule::UpdateToCurrentState()
    {
        switch (CurrentState)
        {
        case State::SignIn:
            UpdateToState("Sign In", true);
            break;
        case State::Play:
            UpdateToState("Play", true, true);
            break;
        case State::Update:
            UpdateToState("Update", true, true);
            break;
        case State::Install:
            UpdateToState("Install", true);
            break;
        case State::SigningIn:
            UpdateToState("Signing In");
            break;
        case State::Playing:
            UpdateToState("Playing");
            break;
        case State::Updating:
            UpdateToState("Updating");
            break;
        case State::Installing:
            UpdateToState("Installing");
            break;
        case State::Uninstalling:
            UpdateToState("Uninstalling");
            break;
        default:
            UpdateToState("Unknown");
            break;
        }
    }

    void GameModule::PlayClicked()
    {
        switch (CurrentState)
        {
        case State::SignIn:
            Auth.StartLogin();

            CurrentState = State::SigningIn;
            UpdateToCurrentState();
            break;
        case State::Play:
            EGL3_LOG(LogLevel::Info, "Play game HYPERS");

            CurrentState = State::Playing;
            UpdateToCurrentState();
            break;
        case State::Update:
            StartUpdate();

            CurrentState = State::Updating;
            UpdateToCurrentState();
            break;
        case State::Install:
        {
            std::filesystem::path InstallFolder;
            if (!GetInstallFolder(InstallFolder)) {
                InstallDialog.Show();
            }
            else {
                StartUpdate();

                CurrentState = State::Installing;
                UpdateToCurrentState();
            }
            break;
        }
        default:
            EGL3_LOG(LogLevel::Warning, "Attempted to click play button on an invalid state");
            break;
        }
    }

    void GameModule::StartUpdate()
    {
        AsyncFF.Enqueue([this]() {
            auto DownloadInfo = Auth.GetClientLauncher().GetDownloadInfo("Windows", "Live", "4fe75bbc5a674f4f9b356b5c90567da5", "Fortnite");
            if (!EGL3_CONDITIONAL_LOG(!DownloadInfo.HasError(), LogLevel::Error, "Download info error")) {
                return;
            }
            auto Element = DownloadInfo->GetElement("Fortnite");
            if (!EGL3_CONDITIONAL_LOG(Element, LogLevel::Error, "No element")) {
                return;
            }

            auto ManifestResp = Web::Epic::EpicClient().GetManifest(Element->PickManifest());
            if (!EGL3_CONDITIONAL_LOG(!ManifestResp.HasError(), LogLevel::Error, "Manifest download error")) {
                return;
            }
            if (!EGL3_CONDITIONAL_LOG(!ManifestResp->HasError(), LogLevel::Error, "Manifest parse error")) {
                return;
            }

            //auto Install = GetInstalls().GetGame(Storage::Game::GameId::Fortnite);
            //EGL3_CONDITIONAL_LOG(Install, LogLevel::Critical, "No game installation found");
            //Updater.QueueUpdate(std::move(ManifestResp.Get()), 30, Install->GetLocation(), FreshInstall ? Storage::Game::ArchiveMode::Create : Storage::Game::ArchiveMode::Load);
        });
    }

    bool GameModule::IsInstalled(Storage::Game::GameId Id) const {
        return false;
    }

    bool GameModule::GetInstallFolder(std::filesystem::path& Path) const {
        std::error_code Error;
        Path = Storage.Get(Storage::Persistent::Key::InstallLocation);
        return std::filesystem::is_directory(Path, Error);
    }
}