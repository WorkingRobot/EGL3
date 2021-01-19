#include "GameInstalls.h"

namespace EGL3::Storage::Models {
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, GameInstalls& Val) {
        Stream >> Val.Installs;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const GameInstalls& Val) {
        Stream << Val.Installs;

        return Stream;
    }

    const GameInstall* GameInstalls::GetGame(Game::GameId Id) const
    {
        auto Itr = std::find_if(Installs.begin(), Installs.end(), [Id](const GameInstall& Install) {
            return Install.GetId() == Id;
        });
        if (Itr != Installs.end()) {
            return &*Itr;
        }
        return nullptr;
    }
}