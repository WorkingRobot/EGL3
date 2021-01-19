#include "GameInstall.h"

namespace EGL3::Storage::Models {
    GameInstall::GameInstall(Game::GameId Id) :
        Id(Id)
    {

    }

    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, GameInstall& Val) {
        uint32_t Id;
        Stream >> Id;
        Val.Id = (Game::GameId)Id;
        std::string Location;
        Stream >> Location;
        Val.Location = Location;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const GameInstall& Val) {
        Stream << (uint32_t)Val.Id;
        Stream << Val.Location.string();

        return Stream;
    }

    Game::GameId GameInstall::GetId() const
    {
        return Id;
    }

    const std::filesystem::path& GameInstall::GetLocation() const
    {
        return Location;
    }

    void GameInstall::SetLocation(const std::filesystem::path& NewLocation)
    {
        Location = NewLocation;
    }
}