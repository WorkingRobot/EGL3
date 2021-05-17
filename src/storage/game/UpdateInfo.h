#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
#pragma pack(push, 2)
    struct UpdateInfo {
        bool IsUpdating;                // Whether this data is valid and the archive is/was undergoing an unfinished update
        uint64_t TargetVersion;         // The "VersionNumeric" that this info was updating to. Throw out this data if a newer update is available
        uint64_t NanosecondsElapsed;    // Nanoseconds that have already been spent on updating
        uint32_t PiecesComplete;        // Total "pieces" already downloaded and installed
        uint64_t BytesDownloadTotal;    // Total bytes that were already downloaded

        UpdateInfo() :
            IsUpdating(false),
            TargetVersion(-1),
            NanosecondsElapsed(0),
            PiecesComplete(0),
            BytesDownloadTotal(0)
        {

        }
    };
#pragma pack(pop)

    static_assert(sizeof(UpdateInfo) == 30);
}