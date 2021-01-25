#pragma once

#include "ManifestData.h"

namespace EGL3::Storage::Game {
    // Still unsure about this data
    struct ManifestData {
        char LaunchExeString[256];      // Path to the executable to launch
        char LaunchCommand[256];        // Extra command arguments to add to the executable
        char AppNameString[64];         // Manifest-specific app name - example: FortniteReleaseBuilds
        uint32_t AppID;                 // Manifest-specific app id - example: 1
        uint32_t BaseUrlCount;          // Base urls (or CloudDirs), used to download chunks
        // A list of zero terminated strings (no alignment)
        // Example: ABCD\0DEFG\0 is a url count of 2 with ABCD and DEFG
        char BaseUrls[];
    };
}