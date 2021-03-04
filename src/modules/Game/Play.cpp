#include "Play.h"

namespace EGL3::Modules::Game {
    using namespace Storage::Models;

    PlayModule::PlayModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        Auth(Modules.GetModule<AuthorizationModule>()),
        Service(Modules.GetModule<ServiceModule>())
    {

    }

    PlayInfo& PlayModule::OnPlayClicked(InstalledGame& Game)
    {
        CurrentPlay = std::make_unique<PlayInfo>(Game);
        CurrentPlay->OnStateUpdate.connect([this](PlayInfoState NewState) {
            switch (NewState)
            {
            case PlayInfoState::Mounting:
                printf("Mounting\n");
                break;
            case PlayInfoState::Playable:
                printf("Playable\n");
                CurrentPlay->Play(Auth.GetClientLauncher());
                break;
            case PlayInfoState::Playing:
                printf("Playing\n");
                break;
            case PlayInfoState::Closed:
                printf("Closed\n");
                break;
            default:
                printf("Unknown\n");
                break;
            }
        });

        CurrentPlay->Mount(Service.GetClient());

        return *CurrentPlay;
    }
}