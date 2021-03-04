#include "MountedGame.h"

#include "../../utils/Assert.h"

namespace EGL3::Storage::Models {
    MountedGame::MountedGame(const std::filesystem::path& Path, Service::Pipe::Client& Client) :
        Path(Path),
        Valid(false),
        Client(Client),
        DriveLetter(0)
    {
        if (!Client.OpenArchive(Path, Context)) {
            return;
        }
        if (!Client.ReadArchive(Context)) {
            return;
        }
        if (!Client.InitializeDisk(Context)) {
            return;
        }
        if (!Client.CreateLUT(Context)) {
            return;
        }
        if (!Client.CreateDisk(Context)) {
            return;
        }
        if (!Client.MountDisk(Context)) {
            return;
        }
        Valid = true;
    }

    MountedGame::~MountedGame()
    {
        if (Context) {
            Client.Destruct(Context);
        }
    }

    bool MountedGame::IsValid() const
    {
        return Valid;
    }

    const std::filesystem::path& MountedGame::GetPath() const
    {
        return Path;
    }

    char MountedGame::GetDriveLetter() const
    {
        if (!DriveLetter) {
            Client.QueryLetter(Context, DriveLetter);
        }

        return DriveLetter;
    }
}