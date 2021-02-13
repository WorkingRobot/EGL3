#pragma once

#include "../bps/Manifest.h"

#include <functional>

namespace EGL3::Web::Epic::Content {
    class LauncherContentStream : public Utils::Streams::Stream {
        friend class LauncherContentClient;

        LauncherContentStream() = default;

        using ChunkDataRequest = std::function<const char*(const Utils::Guid&)>;
        LauncherContentStream(const BPS::Manifest& Manifest, const BPS::FileManifest& File, const ChunkDataRequest& Request);

    public:
        Stream& write(const char* Buf, size_t BufCount) override;

        Stream& read(char* Buf, size_t BufCount) override;

        Stream& seek(size_t Position, SeekPosition SeekFrom) override;

        size_t tell() const override;

        size_t size() const override;

    private:
        bool GetPartIndex(uint64_t ByteOffset, uint32_t& PartIndex, uint32_t& PartByteOffset) const;

        const BPS::Manifest* Manifest;
        const BPS::FileManifest* File;
        ChunkDataRequest Request;

        size_t Position;
    };
}