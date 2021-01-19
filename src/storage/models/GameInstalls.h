#pragma once

#include "../../utils/streams/Stream.h"
#include "GameInstall.h"

namespace EGL3::Storage::Models {
    class GameInstalls {
        std::vector<GameInstall> Installs;

    public:
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, GameInstalls& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const GameInstalls& Val);

        const GameInstall* GetGame(Game::GameId Id) const;
    };
}