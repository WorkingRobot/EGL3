#pragma once

#include "../../utils/stream/CStream.h"

namespace EGL3::Storage::Game {
	struct InstallChunkPart {
		uint64_t Id;					// ID of the chunk, used to calculate the offset where it can be found in the chunk info runlist
		uint32_t Offset;				// Offset of the chunk data to start reading from
		uint32_t Size;					// Number of bytes of the chunk data to read from

		friend CStream& operator>>(CStream& Stream, InstallChunkPart& ChunkPart) {
			Stream >> ChunkPart.Id;
			Stream >> ChunkPart.Offset;
			Stream >> ChunkPart.Size;

			return Stream;
		}

		friend CStream& operator<<(CStream& Stream, InstallChunkPart& ChunkPart) {
			Stream << ChunkPart.Id;
			Stream << ChunkPart.Offset;
			Stream << ChunkPart.Size;

			return Stream;
		}
	};
}