#include "Play.h"

namespace EGL3::Modules::Game {
    using namespace Storage::Models;

    PlayModule::PlayModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        Auth(Modules.GetModule<AuthorizationModule>()),
        Service(Modules.GetModule<ServiceModule>()),
        PlayQueued(false)
    {

    }

    PlayInfo& PlayModule::OnPlayClicked(InstalledGame& Game)
    {
        PlayQueued = true;

        if (!CurrentPlay) {
            CurrentPlay = std::make_unique<PlayInfo>(Game);
            CurrentPlay->OnStateUpdate.connect([this](PlayInfoState NewState) {
                switch (NewState)
                {
                case PlayInfoState::Constructed:
                    printf("Constructed\n");
                    break;
                case PlayInfoState::Mounting:
                    printf("Mounting\n");
                    break;
                case PlayInfoState::Playable:
                    printf("Playable\n");
                    if (PlayQueued.exchange(false)) {
                        OnStateUpdate(true);
                        CurrentPlay->Play(Auth.GetClientLauncher());
                    }
                    else {
                        OnStateUpdate(false);
                    }
                    break;
                case PlayInfoState::Playing:
                    printf("Playing\n");
                    break;
                default:
                    printf("Unknown\n");
                    break;
                }
            });
            CurrentPlay->Mount(Service.GetClient());
        }

        return *CurrentPlay;
    }
}