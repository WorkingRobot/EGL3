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
        PlayBtn.signal_clicked().connect([this]() { PlayClicked(); });

        InstallDialog.LocationChosen.Set([this](const std::string& File) { Install(File); });

        CurrentStateDispatcher.connect([this]() { UpdateToCurrentState(); });

        Auth.AuthChanged.connect(sigc::mem_fun(*this, &GameModule::OnAuthChanged));

        UpdateToCurrentState();
    }

    void GameModule::OnAuthChanged()
    {
        auto Install = GetInstalls().GetGame(Storage::Game::GameId::Fortnite);
        if (Install && std::filesystem::is_regular_file(Install->GetLocation())) {
            CurrentState = State::Play;
        }
        else {
            CurrentState = State::Install;
        }
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
            StartUpdate(false);

            CurrentState = State::Updating;
            UpdateToCurrentState();
            break;
        case State::Install:
        {
            InstallDialog.Show();
            break;
        }
        default:
            EGL3_LOG(LogLevel::Warning, "Attempted to click play button on an invalid state");
            break;
        }
    }

    void GameModule::Install(const std::string& Filename)
    {
        if (CurrentState != State::Install || !Auth.IsLoggedIn()) {
            return;
        }

        GetInstalls().GetOrCreateGame(Storage::Game::GameId::Fortnite).SetLocation(Filename);
        StartUpdate(true);

        CurrentState = State::Installing;
        UpdateToCurrentState();
    }

    void GameModule::StartUpdate(bool FreshInstall) {
        AsyncFF.Enqueue([this, FreshInstall]() {
            auto DownloadInfo = Auth.GetClientLauncher().GetDownloadInfo("Windows", "Live", "4fe75bbc5a674f4f9b356b5c90567da5", "Fortnite");
            if (!EGL3_CONDITIONAL_LOG(!DownloadInfo.HasError(), LogLevel::Error, "Download info error")) {
                return;
            }
            auto Element = DownloadInfo->GetElement("Fortnite");
            if (!EGL3_CONDITIONAL_LOG(Element, LogLevel::Error, "No element")) {
                return;
            }

            auto Manifest = Web::Epic::EpicClient().GetManifest(Element->PickManifest());
            if (!EGL3_CONDITIONAL_LOG(!Manifest.HasError(), LogLevel::Error, "Manifest download error")) {
                return;
            }
            if (!EGL3_CONDITIONAL_LOG(!Manifest->HasError(), LogLevel::Error, "Manifest parse error")) {
                return;
            }

            auto Install = GetInstalls().GetGame(Storage::Game::GameId::Fortnite);
            EGL3_CONDITIONAL_LOG(Install, LogLevel::Critical, "No game installation found");
            if (FreshInstall) {
                Storage::Game::Archive Archive(Install->GetLocation(), Storage::Game::ArchiveOptionCreate);
                Updater.QueueUpdate(std::move(Archive), std::move(Manifest.Get()), 30);
            }
            else {
                Storage::Game::Archive Archive(Install->GetLocation(), Storage::Game::ArchiveOptionLoad);
                Updater.QueueUpdate(std::move(Archive), std::move(Manifest.Get()), 30);
            }
        });
    }

    const Storage::Models::GameInstalls& GameModule::GetInstalls() const
    {
        return Storage.Get(Storage::Persistent::Key::GameInstalls);
    }

    Storage::Models::GameInstalls& GameModule::GetInstalls()
    {
        return Storage.Get(Storage::Persistent::Key::GameInstalls);
    }
}