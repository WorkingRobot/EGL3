#pragma once

#include <stdint.h>

namespace EGL3::Web::Epic::BPS {
    enum class ChunkHashFlags : uint8_t {
		None = 0x00,
		// Flag for FRollingHash class used, stored in RollingHash on header.
		RollingPoly64 = 0x01,
		// Flag for FSHA1 class used, stored in SHAHash on header.
		Sha1 = 0x02,
    };
}