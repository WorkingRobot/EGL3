#pragma once

#include <stdint.h>

namespace EGL3::Utils {
	void SHA1(const char* Buffer, size_t BufferSize, char Dst[20]);

	bool SHA1Verify(const char* Buffer, size_t BufferSize, const char ExpectedHash[20]);
}
