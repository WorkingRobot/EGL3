#include "Play.h"

namespace EGL3::Modules::Game {
    using namespace Storage::Models;

    PlayModule::PlayModule(ModuleList& Ctx) :
        Storage(Ctx.GetStorage()),
        Auth(Ctx.GetModule<Login::AuthModule>()),
        Service(Ctx.GetModule<ServiceModule>()),
        PlayQueued(false)
    {
        PlayQueuedDispatcher.connect([this]() {
            if (PlayQueued.exchange(false)) {
                OnStateUpdate(true);
                CurrentPlay->Play(Auth.GetClientLauncher());
            }
            else {
                OnStateUpdate(false);
            }
        });
    }

    PlayInfo& PlayModule::OnPlayClicked(InstalledGame& Game)
    {
        if (!CurrentPlay) {
            PlayQueued = true;

            CurrentPlay = std::make_unique<PlayInfo>(Game);
            CurrentPlay->OnStateUpdate.connect([this](PlayInfoState NewState) {
                if (NewState == PlayInfoState::Playable) {
                    PlayQueuedDispatcher.emit();
                }
            });
            CurrentPlay->Mount(Service.GetClient());
        }
        else {
            OnStateUpdate(true);
            CurrentPlay->Play(Auth.GetClientLauncher());
        }

        return *CurrentPlay;
    }
}