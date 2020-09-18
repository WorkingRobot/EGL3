#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace EGL3::Utils {
	static void GenerateRandomGuid(uint8_t Guid[16]) {
		for (int i = 0; i < 16; ++i) {
			*Guid++ = rand() & 0xFF;
		}
	}
}