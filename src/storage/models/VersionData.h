#pragma once

#include "../../web/epic/responses/GetDownloadInfo.h"

namespace EGL3::Storage::Models {
    struct VersionData {
        uint64_t VersionNum;
        std::string VersionHR;
        Web::Epic::Responses::GetDownloadInfo::Element Element;
    };
}