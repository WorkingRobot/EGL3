#pragma once

#include "../../../utils/streams/Stream.h"
#include "FeatureLevel.h"
#include "StorageFlags.h"

namespace EGL3::Web::Epic::BPS {
    struct ManifestHeader {
        uint32_t Magic;
        FeatureLevel Version;
        uint32_t HeaderSize;
        uint32_t DataSizeCompressed;
        uint32_t DataSizeUncompressed;
        StorageFlags StoredAs;
        char SHAHash[20];

        static constexpr uint32_t ExpectedMagic = 0x44BEC00C;

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, ManifestHeader& Val);

        bool IsCompressed() const;

        bool IsEncrypted() const;
    };
}