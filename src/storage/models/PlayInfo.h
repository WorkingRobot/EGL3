#pragma once

#include "../../storage/models/InstalledGame.h"

namespace EGL3::Storage::Models {
    enum class PlayInfoState : uint8_t {
        Reading,
        Mounting,
        Playable,
        Playing,
        Closed
    };

    class PlayInfo {
    public:
        PlayInfo(const InstalledGame& Game);

        ~PlayInfo();

    private:
        const InstalledGame& Game;

        PlayInfoState CurrentState;
    };
}