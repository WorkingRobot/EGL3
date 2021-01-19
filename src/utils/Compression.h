#pragma once

#include <stdint.h>

namespace EGL3::Utils {
	enum class CompressionMethod : uint8_t {
		Oodle,
		Zlib,

		Unknown
	};

	template<CompressionMethod Method>
	struct Compressor {
		
	};

	template<>
	struct Compressor<CompressionMethod::Zlib> {
		static bool Decompress(char* Dst, size_t DstSize, const char* Src, size_t SrcSize);
	};
}
