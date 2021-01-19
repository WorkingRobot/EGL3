#pragma once

#include "FeatureLevel.h"
#include "UEStream.h"

namespace EGL3::Web::Epic::BPS {
    enum class ManifestMetaVersion : uint8_t {
        Original,
        SerializesBuildId,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    struct ManifestMeta {
        FeatureLevel FeatureLevel;
        bool IsFileData;
        uint32_t AppId;
        std::string AppName;
        std::string BuildVersion;
        std::string LaunchExe;
        std::string LaunchCommand;
        std::vector<std::string> PrereqIds;
        std::string PrereqName;
        std::string PrereqPath;
        std::string PrereqArgs;
        std::string BuildId;

        friend UEStream& operator>>(UEStream& Stream, ManifestMeta& Val);

        std::string GetBackwardsCompatibleBuildId() const;
    };
}