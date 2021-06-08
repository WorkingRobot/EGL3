#include "Game.h"

#include "../../storage/game/Archive.h"
#include "../../utils/Log.h"
#include "../../web/epic/EpicClient.h"

#include <charconv>
#include <regex>

namespace EGL3::Modules::Game {
    constexpr Storage::Game::GameId PrimaryGame = Storage::Game::GameId::Fortnite;

    GameModule::GameModule(ModuleList& Ctx) :
        Storage(Ctx.GetStorage()),
        AsyncFF(Ctx.GetModule<AsyncFFModule>()),
        Auth(Ctx.GetModule<Login::AuthModule>()),
        Download(Ctx.GetModule<DownloadModule>()),
        Play(Ctx.GetModule<PlayModule>()),
        UpdateCheck(Ctx.GetModule<UpdateCheckModule>()),
        PlayBtn(Ctx.GetWidget<Gtk::Button>("PlayBtn")),
        PlayMenuBtn(Ctx.GetWidget<Gtk::MenuButton>("PlayDropdown")),
        PlayMenuVerifyOpt(Ctx.GetWidget<Gtk::MenuItem>("ExtraPlayVerifyOpt")),
        PlayMenuModifyOpt(Ctx.GetWidget<Gtk::MenuItem>("ExtraPlayModifyOpt")),
        PlayMenuSignOutOpt(Ctx.GetWidget<Gtk::MenuItem>("ExtraPlaySignOutOpt")),
        CurrentStateHolder(),
        InstallStateHolder(*this),
        PlayStateHolder(*this)
    {
        CleanInstalls();

        {
            auto Install = GetInstall(PrimaryGame);
            if (Install) {
                if (Install->GetHeader()->GetUpdateInfo().IsUpdating) {
                    // There was currently an update
                    // If there is a more recent one, then that's fine
                    InstallStateHolder.SetHeldState(State::Update);
                }
                else {
                    PlayStateHolder.SetHeldState(State::Play);
                }
            }
            else {
                InstallStateHolder.SetHeldState(State::Install);
            }
        }

        InstallStateHolder.Clicked.Set([this]() {
            auto& Info = Download.OnDownloadClicked(PrimaryGame);
            Info.OnStateUpdate.connect([this](Storage::Models::DownloadInfoState NewState) {
                switch (NewState)
                {
                case Storage::Models::DownloadInfoState::Initializing:
                    InstallStateHolder.SetHeldState(InstallStateHolder.GetHeldState() == State::Update ? State::Updating : State::Installing);
                    break;
                case Storage::Models::DownloadInfoState::Finished:
                    if (!PlayStateHolder.HasHeldState()) {
                        PlayStateHolder.SetHeldState(State::Play);
                    }
                    InstallStateHolder.ClearHeldState();
                    break;
                case Storage::Models::DownloadInfoState::Cancelled:
                    InstallStateHolder.SetHeldState(InstallStateHolder.GetHeldState() == State::Updating ? State::Update : State::Install);
                    break;
                }
            });
        });

        PlayStateHolder.Clicked.Set([this]() {
            auto Install = GetInstall(PrimaryGame);
            EGL3_VERIFY(Install, "No game, but playable?");
            Play.OnPlayClicked(*Install);

            PlayStateHolder.SetHeldState(State::Playing);
        });

        UpdateCheck.OnUpdateAvailable.connect([this](Storage::Game::GameId Id, const Storage::Models::VersionData& Data) {
            if (Id == PrimaryGame) {
                InstallStateHolder.SetHeldState(State::Update);
                EGL3_LOGF(LogLevel::Info, "Update available to {} ({})", Data.VersionHR, Data.VersionNum);
            }
        });

        Play.OnStateUpdate.Set([this](bool Playing) {
            PlayStateHolder.SetHeldState(Playing ? State::Playing : State::Play);
        });

        PlayBtn.signal_clicked().connect([this]() { PrimaryButtonClicked(); });

        CurrentStateDispatcher.connect([this]() { OnUpdateToCurrentState(); });

        UpdateToCurrentState();
    }

    void GameModule::UpdateToState(const char* NewLabel, bool Playable, bool Menuable)
    {
        PlayBtn.set_label(NewLabel);
        PlayBtn.set_sensitive(Playable);
        PlayMenuBtn.set_sensitive(Menuable);
    }

    void GameModule::OnUpdateToCurrentState() {
        {
            std::shared_lock Lock(StateHolderMtx);
            if (StateHolders.empty()) {
                CurrentStateHolder = nullptr;
            }
            else {
                CurrentStateHolder = *std::max_element(StateHolders.begin(), StateHolders.end(), StateHolderComparer());
            }
        }

        switch (CurrentStateHolder ? CurrentStateHolder->GetHeldState() : State::Unknown)
        {
        case State::Play:
            UpdateToState("Play", true, true);
            break;
        case State::Update:
            UpdateToState("Update", true, true);
            break;
        case State::Install:
            UpdateToState("Install", true);
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
        case State::Unknown:
        default:
            UpdateToState("Unknown");
            break;
        }
    }

    void GameModule::UpdateToCurrentState()
    {
        CurrentStateDispatcher.emit();
    }

    void GameModule::PrimaryButtonClicked()
    {
        if (CurrentStateHolder) {
            CurrentStateHolder->Clicked();
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

    GameModule::StateHolder::StateHolder(GameModule& Module, State HeldState) :
        Module(&Module),
        HeldState(HeldState)
    {
        {
            std::lock_guard Lock(Module.StateHolderMtx);
            Module.StateHolders.emplace_back(this);
        }
        Module.UpdateToCurrentState();
    }

    GameModule::StateHolder::~StateHolder()
    {
        if (Module) {
            {
                std::lock_guard Lock(Module->StateHolderMtx);
                Module->StateHolders.erase(std::remove(Module->StateHolders.begin(), Module->StateHolders.end(), this), Module->StateHolders.end());
            }
            Module->UpdateToCurrentState();
            Module = nullptr;
        }
    }

    void GameModule::StateHolder::SetHeldState(State NewHeldState)
    {
        if (HeldState != NewHeldState) {
            HeldState = NewHeldState;
            Module->UpdateToCurrentState();
        }
    }

    void GameModule::StateHolder::ClearHeldState()
    {
        SetHeldState(State::Unknown);
    }
}