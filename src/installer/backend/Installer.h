#pragma once

#include "streams/WebStream.h"
#include "Unpacker.h"

#include <filesystem>

namespace EGL3::Installer::Backend {
    class Installer {
    public:
        Installer(const std::filesystem::path& TargetDirectory);

        void Run();

        void GetProgress(float& Value, Unpacker::State& State) const;

        const std::filesystem::path& GetLaunchPath() const;

        Utils::Callback<void()>& OnProgressUpdate;
        Utils::Callback<void(const std::string& Error)>& OnProgressError;

    private:
        Streams::WebStream Stream;
        Unpacker Reader;
    };
}