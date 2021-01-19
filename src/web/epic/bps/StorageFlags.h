#pragma once

#include <stdint.h>

namespace EGL3::Web::Epic::BPS {
    enum class StorageFlags : uint8_t {
        // Stored as raw data.
        None = 0,
        // Flag for compressed data.
        Compressed = 1,
        // Flag for encrypted. If also compressed, decrypt first. Encryption will ruin compressibility.
        Encrypted = 1 << 1
    };
}