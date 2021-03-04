#pragma once

#include "../../srv/pipe/Client.h"

#include <filesystem>

namespace EGL3::Storage::Models {
    class MountedGame {
    public:
        MountedGame(const std::filesystem::path& Path, Service::Pipe::Client& Client);

        ~MountedGame();

        bool IsValid() const;

        const std::filesystem::path& GetPath() const;

        char GetDriveLetter() const;

    private:
        Service::Pipe::Client& Client;

        std::filesystem::path Path;
        bool Valid;
        void* Context;
        mutable char DriveLetter;
    };
}