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

        SlotLogOutPreflight = Auth.LogOutPreflight.connect([this, &Ctx]() {
            if (CurrentPlay) {
                if (CurrentPlay->GetState() == PlayInfoState::Playable) {
                    return true;
                }

                Ctx.DisplayError("You're currently playing Fortnite right now, you can't quit yet.", "<i><small>If you wish to play on multiple accounts,\nhold \"Shift\" when clicking the play button.</small></i>", "Currently Playing", true);
                return false;
            }
            return true;
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