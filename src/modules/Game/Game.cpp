#include "Game.h"

#include "../../storage/game/Archive.h"
#include "../../utils/Log.h"
#include "../../web/epic/EpicClient.h"

#include <charconv>
#include <regex>

namespace EGL3::Modules::Game {
    constexpr Storage::Game::GameId PrimaryGame = Storage::Game::GameId::Fortnite;

    GameModule::GameModule(ModuleList& Ctx) :
        InstalledGames(Ctx.Get<InstalledGamesSetting>()),
        AsyncFF(Ctx.GetModule<AsyncFFModule>()),
        SysTray(Ctx.GetModule<SysTrayModule>()),
        Auth(Ctx.GetModule<Login::AuthModule>()),
        Download(Ctx.GetModule<DownloadModule>()),
        Play(Ctx.GetModule<PlayModule>()),
        UpdateCheck(Ctx.GetModule<UpdateCheckModule>()),
        PlayBtn(Ctx.GetWidget<Gtk::Button>("PlayBtn")),
        PlayMenuBtn(Ctx.GetWidget<Gtk::MenuButton>("PlayDropdown")),
        PlayMenuVerifyOpt(Ctx.GetWidget<Gtk::MenuItem>("ExtraPlayVerifyOpt")),
        PlayMenuModifyOpt(Ctx.GetWidget<Gtk::MenuItem>("ExtraPlayModifyOpt")),
        PlayMenuSignOutOpt(Ctx.GetWidget<Gtk::MenuItem>("ExtraPlaySignOutOpt")),
        ShiftPressed(false),
        CurrentStateHolder(nullptr),
        ConfirmInstallStateHolder(*this),
        InstallStateHolder(*this),
        PlayStateHolder(*this)
    {
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

        ConfirmInstallStateHolder.Clicked.Set([this]() {
            Download.OnDownloadOkClicked([this]() -> Storage::Models::InstalledGame& {
                return InstalledGames->emplace_back();
            });
            ConfirmInstallStateHolder.ClearHeldState();
        });

        InstallStateHolder.Clicked.Set([this]() {
            auto& Info = Download.OnDownloadClicked(PrimaryGame, GetInstall(PrimaryGame));
            Info.OnStateUpdate.connect([this](Storage::Models::DownloadInfoState NewState) {
                switch (NewState)
                {
                case Storage::Models::DownloadInfoState::Initializing:
                    InstallStateHolder.SetHeldState(InstallStateHolder.GetHeldState() == State::Update ? State::Updating : State::Installing);
                    InstalledGames.Flush();
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
            ConfirmInstallStateHolder.SetHeldState(State::Continue);
        });

        PlayStateHolder.Clicked.Set([this]() {
            auto Install = GetInstall(PrimaryGame);
            EGL3_VERIFY(Install, "No game, but playable?");
            Play.OnPlayClicked(*Install, !ShiftPressed);

            PlayStateHolder.SetHeldState(State::Playing);
        });

        UpdateCheck.OnUpdateAvailable.connect([this](Storage::Game::GameId Id, const Storage::Models::VersionData& Data) {
            if (Id == PrimaryGame) {
                InstallStateHolder.SetHeldState(State::Update);
                EGL3_LOGF(LogLevel::Info, "Update available to {} ({})", Data.VersionHR, Data.VersionNum);

                SysTray.ShowToast(Utils::ToastTemplate{
                    .Type = Utils::ToastType::Text02,
                    .TextFields = { "New Fortnite Update", "A new version of Fortnite is available!" },
                    .Actions = { std::format("Update to {}", Data.VersionHR) }
                }, {
                    .OnClicked = [this](int ActionIdx) {
                        if (GetCurrentState() == State::Update) {
                            SysTray.SetAppState(SysTrayModule::AppState::Focused);
                            PrimaryButtonClicked();
                        }
                    }
                });
            }
        });

        Play.OnStateUpdate.Set([this](bool Playing) {
            PlayStateHolder.SetHeldState(Playing ? State::Playing : State::Play);
        });

        SlotPlayClicked = PlayBtn.signal_clicked().connect([this]() { PrimaryButtonClicked(); });

        SlotSysTrayActionClicked = SysTray.OnActionClicked.connect([this]() { PrimaryButtonClicked(); });

        SlotShiftPress = Ctx.GetWidget<Gtk::Window>("EGL3App").signal_key_press_event().connect([this](GdkEventKey* Event) {
            ShiftPressed = Event->state & Gdk::SHIFT_MASK;
            return false;
        });

        SlotShiftRelease = Ctx.GetWidget<Gtk::Window>("EGL3App").signal_key_release_event().connect([this](GdkEventKey* Event) {
            ShiftPressed = Event->state & Gdk::SHIFT_MASK;
            return false;
        });

        CurrentStateDispatcher.connect([this]() { OnUpdateToCurrentState(); });

        UpdateToCurrentState();
    }

    void GameModule::GetStateData(State State, std::string& Label, bool& Playable, bool& Menuable)
    {
        Playable = false;
        Menuable = false;

        switch (State)
        {
        case State::Play:
            Label = "Play";
            Playable = true;
            Menuable = true;
            break;
        case State::Update:
            Label = "Update";
            Playable = true;
            Menuable = true;
            break;
        case State::Install:
            Label = "Install";
            Playable = true;
            break;
        case State::Continue:
            Label = "Continue";
            Playable = true;
            break;
        case State::Playing:
            Label = "Playing";
            break;
        case State::Updating:
            Label = "Updating";
            break;
        case State::Installing:
            Label = "Installing";
            break;
        case State::Uninstalling:
            Label = "Uninstalling";
            break;
        case State::Unknown:
        default:
            Label = "Unknown";
            break;
        }
    }

    GameModule::State GameModule::GetCurrentState() const
    {
        return CurrentStateHolder ? CurrentStateHolder->GetHeldState() : State::Unknown;
    }

    void GameModule::PrimaryButtonClicked()
    {
        if (CurrentStateHolder) {
            CurrentStateHolder->Clicked();
        }
    }

    void GameModule::UpdateToState(const std::string& NewLabel, bool Playable, bool Menuable)
    {
        PlayBtn.set_label(NewLabel);
        PlayBtn.set_sensitive(Playable);
        PlayMenuBtn.set_sensitive(Menuable);

        SysTray.SetActionLabel(NewLabel, Playable);
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

        std::string Label;
        bool Playable;
        bool Menuable;
        GetStateData(GetCurrentState(), Label, Playable, Menuable);
        UpdateToState(Label, Playable, Menuable);
    }

    void GameModule::UpdateToCurrentState()
    {
        CurrentStateDispatcher.emit();
    }

    Storage::Models::InstalledGame* GameModule::GetInstall(Storage::Game::GameId Id) {
        auto Itr = std::find_if(InstalledGames->begin(), InstalledGames->end(), [Id](Storage::Models::InstalledGame& Game) {
            auto HeaderPtr = Game.GetHeader();
            return HeaderPtr ? HeaderPtr->GetGameId() == Id : false;
        });

        if (Itr != InstalledGames->end()) {
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