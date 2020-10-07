#pragma once

#include "../../utils/streams/SyncedFileStream.h"
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
	class Archive {
	public:
		// at offset 0
		const Header* Header;

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

		struct CreationData {
			std::string Game;
			std::string VersionStringLong;
			std::string VersionStringHR;
			GameId GameNumeric;
			uint64_t VersionNumeric;

			std::string LaunchExeString;
			std::string LaunchCommand;
			std::string AppNameString;
			uint32_t AppID;
			std::vector<std::string> BaseUrls;
		};

		static Archive&& Create(const fs::path& Path, CreationData&& Data);

		static Archive&& Open(const fs::path& Path);

		void ModifyMetadata(CreationData&& Data);

		/*template<RunlistId RunlistType>
		RunlistStream OpenRunlist() {

		}*/

		~Archive();

		friend class RunlistStream;

	private:
		Archive(const fs::path& Path);
		
		void ModifyMetadataInternal(CreationData&& Data);

		void Resize(Runlist& Runlist, int64_t NewSize);

		Utils::Mmio::MmioFile Backend;
	};
}