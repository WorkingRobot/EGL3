#pragma once

#include <stdint.h>

namespace EGL3::Web::Epic::BPS {
    enum class ChunkVersion : uint32_t {
		Invalid = 0,
		Original,
		StoresShaAndHashType,
		StoresDataSizeUncompressed,

		// Always after the latest version, signifies the latest version plus 1 to allow initialization simplicity.
		LatestPlusOne,
		Latest = LatestPlusOne - 1
    };
}