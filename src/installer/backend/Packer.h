#pragma once

#include "../../utils/streams/Stream.h"
#include "RegistryInfo.h"

#include <filesystem>

namespace EGL3::Installer::Backend {
    class Packer {
    public:
        Packer(const std::filesystem::path& TargetDirectory, RegistryInfo& Registry, Utils::Streams::Stream& OutStream);
    };
}