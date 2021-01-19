#include "ReadonlyArchive.h"

#include "../../utils/Assert.h"
#include "../../utils/Align.h"
#include "RunlistStream.h"
#include "GameId.h"
#include "RunlistId.h"

namespace EGL3::Storage::Game {
	ReadonlyArchive::ReadonlyArchive(const fs::path& Path) : Backend(Path)
	{
		Header				= (decltype(Header))			Backend.Get() + 0;
		FileRunlist			= (decltype(FileRunlist))		Backend.Get() + Header->FileRunlistOffset;
		ChunkPartRunlist	= (decltype(ChunkPartRunlist))	Backend.Get() + Header->ChunkPartRunlistOffset;
		ChunkInfoRunlist	= (decltype(ChunkInfoRunlist))	Backend.Get() + Header->ChunkInfoRunlistOffset;
		ChunkDataRunlist	= (decltype(ChunkDataRunlist))	Backend.Get() + Header->ChunkDataRunlistOffset;
		RunIndex			= (decltype(RunIndex))			Backend.Get() + Header->RunIndexOffset;
		ManifestData		= (decltype(ManifestData))		Backend.Get() + Header->ManifestDataOffset;
	}

	ReadonlyArchive::~ReadonlyArchive()
	{
	}
}
