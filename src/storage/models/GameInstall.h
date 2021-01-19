#pragma once

#include "../../utils/streams/Stream.h"
#include "../game/GameId.h"

#include <filesystem>

namespace EGL3::Storage::Models {
    class GameInstall {
        Game::GameId Id;
        std::filesystem::path Location;

    public:
        GameInstall() = default;

        GameInstall(Game::GameId Id);
        
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, GameInstall& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const GameInstall& Val);

        Game::GameId GetId() const;

        const std::filesystem::path& GetLocation() const;

        void SetLocation(const std::filesystem::path& NewLocation);
    };
}