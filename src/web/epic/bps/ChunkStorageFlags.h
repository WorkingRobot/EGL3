#pragma once

#include <stdint.h>

namespace EGL3::Web::Epic::BPS {
    enum class ChunkStorageFlags : uint8_t {
        None = 0x00,
        // Flag for compressed data.
        Compressed = 0x01,
        // Flag for encrypted. If also compressed, decrypt first. Encryption will ruin compressibility.
        Encrypted = 0x02
    };
}