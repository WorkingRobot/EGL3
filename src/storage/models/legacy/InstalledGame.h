#pragma once

#include "../../../utils/streams/Stream.h"

#include <filesystem>

namespace EGL3::Storage::Models::Legacy {
    struct InstalledGame {
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, InstalledGame& Val)
        {
            std::string Path;
            Stream >> Path;
            Val.Path = Path;
            Stream >> Val.Flags;

            return Stream;
        }

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const InstalledGame& Val)
        {
            Stream << Val.Path.string();
            Stream << Val.Flags;

            return Stream;
        }

        std::filesystem::path Path;
        uint16_t Flags;
    };
}