#pragma once

#include "../../utils/mmio/MmioFile.h"
#include "GameId.h"
#include "Header.h"
#include "Runlist.h"
#include "ManifestData.h"
#include "RunIndex.h"

#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace EGL3::Storage::Game {
	// A full Fortnite (or maybe other game) installation
	// As a disclaimer, we assume that all structs are aligned to 8 bytes (in order for this to compile without UB)
	class Archive {
	public:
		Utils::Mmio::MmioFile Backend;

		// at offset 0
		Header* Header;

		// at offset 256
		Runlist* FileRunlist;

		// at offset 512
		Runlist* ChunkPartRunlist;

		// at offset 768
		Runlist* ChunkInfoRunlist;

		// at offset 1024
		Runlist* ChunkDataRunlist;

		// at offset 2048
		ManifestData* ManifestData;

		// at offset 4096
		RunIndex* RunIndex;

		// at offset 8192 (sector 16)
		// technical start of file data and all

		Archive(const fs::path& Path);

		~Archive();

	private:
		void Resize(Runlist* Runlist, int64_t NewSize);

		friend class RunlistStream;
	};
}