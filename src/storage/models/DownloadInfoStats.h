#pragma once

#include <stdint.h>

namespace EGL3::Storage::Models {
    enum class DownloadInfoState : uint8_t {
        Options,
        Initializing,
        Installing,
        Finishing,
        Finished,
        Paused,
        Cancelling,
        Cancelled
    };

    struct DownloadInfoStats {
        DownloadInfoState State;
        uint64_t NanosecondStartTimestamp;
        uint64_t NanosecondCurrentTimestamp;
        uint32_t PiecesTotal;
        uint64_t DownloadTotal;

        uint32_t PiecesComplete;
        uint64_t BytesDownloadTotal;
        uint64_t BytesDownloadRate;
        uint64_t BytesReadRate;
        uint64_t BytesWriteRate;
    };

    static constexpr const char* DownloadInfoStateToString(DownloadInfoState State) {
        switch (State)
        {
        case DownloadInfoState::Options:
            return "Configuring";
        case DownloadInfoState::Initializing:
            return "Initializing";
        case DownloadInfoState::Installing:
            return "Installing";
        case DownloadInfoState::Finishing:
            return "Finishing";
        case DownloadInfoState::Finished:
            return "Finished";
        case DownloadInfoState::Paused:
            return "Paused";
        case DownloadInfoState::Cancelling:
            return "Cancelling";
        case DownloadInfoState::Cancelled:
            return "Cancelled";
        default:
            return "Unknown";
        }
    }
}