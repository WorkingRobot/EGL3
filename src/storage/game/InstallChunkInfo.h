#pragma once

#include "../../utils/stream/CStream.h"

namespace EGL3::Storage::Game {
	struct InstallChunkInfo {
		char SHA[20];					// SHA hash of the chunk to check against
		uint32_t UncompressedSize;		// Uncompressed, total size that the chunk can provide (a value of 0xFFFFFFFF means uncompressed)
		uint32_t DataSize;				// Physical size of the chunk in the data runlist (this can mean compressed size, or uncompressed size if the chunk is uncompressed)
		uint32_t DataSector;			// Sector of where the chunk data starts. This is the 512-byte offset of the data in the ChunkDataRunlist (not a sector like in a runlist exactly!)
		char Guid[16];					// GUID of the chunk, used in EGL's manifests, unique
		uint64_t Hash;					// Hash of the chunk, used in EGL's manifests, used to get chunk url
		uint8_t DataGroup;				// Data group of the chunk, used in EGL's manifests, used to get chunk url
		// 1 byte padding
		uint16_t Flags;					// Flags of the chunk, marks multiple things, like compression method
		// 4 byte padding (8 byte alignment)

		friend CStream& operator>>(CStream& Stream, InstallChunkInfo& ChunkInfo) {
			Stream >> ChunkInfo.SHA;
			Stream >> ChunkInfo.UncompressedSize;
			Stream >> ChunkInfo.DataSize;
			Stream >> ChunkInfo.DataSector;
			Stream >> ChunkInfo.Guid;
			Stream >> ChunkInfo.Hash;
			Stream >> ChunkInfo.DataGroup;
			Stream.seek(1, CStream::Cur);
			Stream >> ChunkInfo.Flags;
			Stream.seek(4, CStream::Cur);

			return Stream;
		}

		friend CStream& operator<<(CStream& Stream, InstallChunkInfo& ChunkInfo) {
			Stream << ChunkInfo.SHA;
			Stream << ChunkInfo.UncompressedSize;
			Stream << ChunkInfo.DataSize;
			Stream << ChunkInfo.DataSector;
			Stream << ChunkInfo.Guid;
			Stream << ChunkInfo.Hash;
			Stream << ChunkInfo.DataGroup;
			Stream.seek(1, CStream::Cur);
			Stream << ChunkInfo.Flags;
			Stream.seek(4, CStream::Cur);

			return Stream;
		}
	};
}