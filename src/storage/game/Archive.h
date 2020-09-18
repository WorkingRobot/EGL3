#pragma once

#include "../../utils/streams/FileStream.h"
#include "Header.h"
#include "Runlist.h"
#include "ManifestData.h"
#include "RunIndex.h"

#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace EGL3::Storage::Game {
	// A full Fortnite (or maybe other game) installation
	class Archive {
	public:
		// at offset 0
		Header Header;

		// at offset 256
		Runlist FileRunlist;

		// at offset 512
		Runlist ChunkPartRunlist;

		// at offset 768
		Runlist ChunkInfoRunlist;

		// at offset 1024
		Runlist ChunkDataRunlist;

		// at offset 2048
		ManifestData ManifestData;

		// at offset 4096
		RunIndex RunIndex;

		// at offset 8192 (sector 16)
		// technical start of file data and all

		Archive(fs::path Path);
		~Archive();

		friend class RunlistStream;

	private:
		void InitializeParse();
		void InitializeCreate();

		void Resize(Runlist& Runlist, int64_t NewSize);

		Utils::Streams::FileStream Stream;
	};
}