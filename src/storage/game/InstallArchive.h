#pragma once

#include "../utils/stream/CFileStream.h"
#include "InstallHeader.h"
#include "InstallRunlist.h"
#include "InstallManifestData.h"
#include "InstallRunIndex.h"

#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace EGL3::Storage::Game {
	// A full Fortnite (or maybe other game) installation
	class InstallArchive {
	public:
		// at offset 0
		InstallHeader Header;

		// at offset 256
		InstallRunlist FileRunlist;

		// at offset 512
		InstallRunlist ChunkPartRunlist;

		// at offset 768
		InstallRunlist ChunkInfoRunlist;

		// at offset 1024
		InstallRunlist ChunkDataRunlist;

		// at offset 2048
		InstallManifestData ManifestData;

		// at offset 4096
		InstallRunIndex RunIndex;

		// at offset 8192 (sector 16)
		// technical start of file data and all

		InstallArchive(fs::path Path);
		~InstallArchive();

		friend class CRunlistStream;

	private:
		void InitializeParse();
		void InitializeCreate();

		void Resize(InstallRunlist& Runlist, int64_t NewSize);

		CFileStream Stream;
	};
}