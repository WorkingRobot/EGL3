#pragma once

#include "../../utils/streams/Stream.h"
#include "VersionInfo.h"

#include <filesystem>

namespace EGL3::Installer::Backend {
    class Packer {
    public:
        Packer(const std::filesystem::path& TargetDirectory, VersionInfo& Version, Utils::Streams::Stream& OutEgluStream, Utils::Streams::Stream& OutJsonStream);
    };
}