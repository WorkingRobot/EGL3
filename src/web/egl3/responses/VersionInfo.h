#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::EGL3::Responses {
    struct VersionInfo {
        std::string Version;
        std::string VersionHR;
        uint64_t VersionNum;
        TimePoint ReleaseDate;
        std::string PatchNotes;

        PARSE_DEFINE(VersionInfo)
            PARSE_ITEM("version", Version)
            PARSE_ITEM("versionHR", VersionHR)
            PARSE_ITEM("versionNum", VersionNum)
            PARSE_ITEM("releaseDate", ReleaseDate)
            PARSE_ITEM("patchNotes", PatchNotes)
        PARSE_END
    };
}
