#include "Game.h"

#include "../../storage/game/Archive.h"
#include "../../utils/Assert.h"
#include "../../web/epic/EpicClient.h"

namespace EGL3::Modules::Game {
    GameModule::GameModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        AsyncFF(Modules.GetModule<AsyncFFModule>()),
        Auth(Modules.GetModule<AuthorizationModule>()),
        PlayBtn(Builder.GetWidget<Gtk::Button>("PlayBtn")),
        PlayMenuBtn(Builder.GetWidget<Gtk::MenuButton>("PlayDropdown")),
        PlayMenuVerifyOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlayVerifyOpt")),
        PlayMenuModifyOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlayModifyOpt")),
        PlayMenuSignOutOpt(Builder.GetWidget<Gtk::MenuItem>("ExtraPlaySignOutOpt")),
        CurrentState(State::SignIn),
        InstallDialog(Builder.GetWidget<Gtk::ApplicationWindow>("EGL3App"))
    {
        PlayBtn.signal_clicked().connect([this]() { PlayClicked(); });

        InstallDialog.LocationChosen.Set([this](const std::string& File) { CreateInstall(File); });

        CurrentStateDispatcher.connect([this]() { UpdateToCurrentState(); });

        Auth.AuthChanged.connect(sigc::mem_fun(*this, &GameModule::OnAuthChanged));

        UpdateToCurrentState();
    }

    void GameModule::OnAuthChanged()
    {
        auto Install = GetInstall();
        if (!Install) {
            CurrentState = State::Install;
        }
        else {
            // Check for update here :)
            CurrentState = State::Play;
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
            // Mount to current game, update

            CurrentState = State::Updating;
            UpdateToCurrentState();
            break;
        case State::Install:
        {
            InstallDialog.Show();

            //Auth.GetClientLauncher().GetDownloadInfo("Windows", "Live", "4fe75bbc5a674f4f9b356b5c90567da5", "Fortnite");
            //CurrentState = State::Installing;
            //UpdateToCurrentState();
            break;
        }
        default:
            EGL3_LOG(LogLevel::Warning, "Attempted to click play button on an invalid state");
            break;
        }
    }

    std::unique_ptr<char[]> ReadBytes(const fs::path& Path, size_t& OutSize) {
        auto File = fopen(Path.string().c_str(), "rb");
        OutSize = fs::file_size(Path);
        auto Ret = std::make_unique<char[]>(OutSize);
        fread(Ret.get(), OutSize, 1, File);
        fclose(File);
        return Ret;
    }

    void GameModule::CreateInstall(const std::string& Filename)
    {
        if (CurrentState != State::Install || !Auth.IsLoggedIn()) {
            return;
        }
        CurrentState = State::Installing;
        UpdateToCurrentState();
        AsyncFF.Enqueue([this]() {
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
            for (auto& File : Manifest->FileManifestList.FileList) {
                printf("%s (%d bytes)\n", File.Filename.c_str(), File.FileSize);
            }
            for (auto& Field : Manifest->CustomFields.Fields) {
                printf("%s - %s\n", Field.first.c_str(), Field.second.c_str());
            }
        });
    }

    const Storage::Models::GameInstall* GameModule::GetInstall() const
    {
        return Storage.Get(Storage::Persistent::Key::GameInstalls).GetGame(Storage::Game::GameId::Fortnite);
    }
}