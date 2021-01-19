#pragma once

#include "FileManifest.h"
#include "UEStream.h"

namespace EGL3::Web::Epic::BPS {
    enum class FileManifestListVersion : uint8_t {
        Original,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    struct FileManifestList {
        std::vector<FileManifest> FileList;

        friend UEStream& operator>>(UEStream& Stream, FileManifestList& Val);

        void OnPostLoad();
    };
}