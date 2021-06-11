#pragma once

#include <filesystem>

#include <guiddef.h>
#include <KnownFolders.h>

namespace EGL3::Utils::Platform {
	std::filesystem::path GetKnownFolderPath(const GUID& FolderId);
}