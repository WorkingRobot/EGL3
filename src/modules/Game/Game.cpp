#include "Game.h"

#include "../../storage/game/Archive.h"
#include "../../utils/Assert.h"
#include "../../web/epic/EpicClient.h"

namespace EGL3::Modules::Game {
    GameModule::GameModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        AsyncFF(Modules.GetModule<AsyncFFModule>()),
        Auth(Modules.GetModule<AuthorizationModule>()),
        Download(Modules.GetModule<DownloadModule>()),
        PlayBtn(Builder.GetWidget<Gtk::Button>("PlayBtn")),
        PlayMenuBtn(Builder.GetWidget<Gtk::MenuButton>("PlayDropdown")),
        PlayMenuVerifyOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlayVerifyOpt")),
        PlayMenuModifyOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlayModifyOpt")),
        PlayMenuSignOutOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlaySignOutOpt")),
        CurrentState(State::SignIn)
    {
        CleanInstalls();

        PlayBtn.signal_clicked().connect([this]() { PrimaryButtonClicked(); });

        CurrentStateDispatcher.connect([this]() { UpdateToCurrentState(); });

        Auth.AuthChanged.connect(sigc::mem_fun(*this, &GameModule::OnAuthChanged));

        UpdateToCurrentState();
    }

    void GameModule::OnAuthChanged()
    {
        auto Install = GetInstall(Storage::Game::GameId::Fortnite);
        if (Install) {
            if (Install->GetHeader()->GetUpdateInfo().IsUpdating) {
                // There was currently an update
                // If there is a more recent one, then that's fine
                CurrentState = State::Update;
            }
            else {
                CurrentState = State::Play;
            }
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

    void GameModule::SetCurrentState(State NewState)
    {
        CurrentState = NewState;

        CurrentStateDispatcher.emit();
    }

    void GameModule::UpdateToCurrentState() {
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

    void GameModule::PrimaryButtonClicked()
    {
        switch (CurrentState)
        {
        case State::SignIn:
            Auth.StartLogin();

            SetCurrentState(State::SigningIn);
            break;
        case State::Play:
            EGL3_LOG(LogLevel::Info, "Play game HYPERS");

            SetCurrentState(State::Playing);
            break;
        case State::Update:
        case State::Install:
        {
            auto& Info = Download.OnDownloadClicked(Storage::Game::GameId::Fortnite);
            Info.OnStateUpdate.connect([this](Storage::Models::DownloadInfoState NewState) {
                switch (NewState)
                {
                case Storage::Models::DownloadInfoState::Initializing:
                    SetCurrentState(CurrentState == State::Update ? State::Updating : State::Installing);
                    break;
                case Storage::Models::DownloadInfoState::Finished:
                    SetCurrentState(State::Play);
                    break;
                // case Storage::Models::DownloadInfo::State::Cancelling:
                case Storage::Models::DownloadInfoState::Cancelled:
                    SetCurrentState(CurrentState == State::Updating ? State::Update : State::Install);
                    break;
                }
            });
            break;
        }
        default:
            EGL3_LOG(LogLevel::Warning, "Attempted to click play button on an invalid state");
            break;
        }
    }

    void GameModule::CleanInstalls()
    {
        auto& InstalledGames = Storage.Get(Storage::Persistent::Key::InstalledGames);
        std::unordered_map<std::filesystem::path::string_type, Storage::Models::InstalledGame> Games;
        InstalledGames.reserve(InstalledGames.size());
        for (auto& Game : InstalledGames) {
            if (Game.IsValid()) {
                Games.try_emplace(std::filesystem::absolute(Game.GetPath()), std::move(Game));
            }
        }

        InstalledGames.clear();
        for (auto& Game : Games) {
            auto& NewGame = InstalledGames.emplace_back(std::move(Game.second));
            NewGame.SetPath(Game.first);
        }

        Storage.Flush();
    }

    Storage::Models::InstalledGame* GameModule::GetInstall(Storage::Game::GameId Id) {
        auto& InstalledGames = Storage.Get(Storage::Persistent::Key::InstalledGames);
        auto Itr = std::find_if(InstalledGames.begin(), InstalledGames.end(), [Id](Storage::Models::InstalledGame& Game) {
            if (auto HeaderPtr = Game.GetHeader()) {
                return HeaderPtr->GetGameId() == Id;
            }
            return false;
        });

        if (Itr != InstalledGames.end()) {
            return &*Itr;
        }
        return nullptr;
    }
}