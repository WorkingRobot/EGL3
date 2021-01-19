#pragma once

#include "ChunkPart.h"
#include "FileMetaFlags.h"

#include <vector>

namespace EGL3::Web::Epic::BPS {
    struct FileManifest {
        std::string Filename;
        std::string SymlinkTarget;
        char FileHash[20];
        FileMetaFlags FileMetaFlags;
        std::vector<std::string> InstallTags;
        std::vector<ChunkPart> ChunkParts;
        uint64_t FileSize;

        std::weak_ordering operator<=>(const FileManifest& that) const;
    };
}