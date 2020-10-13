#pragma once

#include <stdint.h>

namespace EGL3::Storage::Game {
	struct ChunkPart {
		uint64_t Id;					// ID of the chunk, used to calculate the offset where it can be found in the chunk info runlist
		uint32_t Offset;				// Offset of the chunk data to start reading from
		uint32_t Size;					// Number of bytes of the chunk data to read from
	};
}