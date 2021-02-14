#include "PlayInfo.h"

namespace EGL3::Storage::Models {
    PlayInfo::PlayInfo(const InstalledGame& Game) :
        Game(Game),
        CurrentState(PlayInfoState::Reading)
    {
    }

    PlayInfo::~PlayInfo()
    {

    }
}