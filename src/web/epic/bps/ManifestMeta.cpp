#include "ManifestMeta.h"

#include "../../../utils/Log.h"
#include "../../../utils/Base64.h"
#include "../../../utils/SHABuilder.h"

#include <algorithm>

namespace EGL3::Web::Epic::BPS {
    UEStream& operator>>(UEStream& Stream, ManifestMeta& Val) {
        auto StartPos = Stream.tell();

        uint32_t DataSize;
        ManifestMetaVersion DataVersion;
        {
            Stream >> DataSize;
            uint8_t DataVersionRaw;
            Stream >> DataVersionRaw;
            DataVersion = (ManifestMetaVersion)DataVersionRaw;
        }

        if (DataVersion >= ManifestMetaVersion::Original) {
            int32_t FeatureLevel;
            Stream >> FeatureLevel;
            Val.FeatureLevel = (BPS::FeatureLevel)FeatureLevel;
            Stream >> Val.IsFileData;
            Stream >> Val.AppId;
            Stream >> Val.AppName;
            Stream >> Val.BuildVersion;
            Stream >> Val.LaunchExe;
            Stream >> Val.LaunchCommand;
            Stream >> Val.PrereqIds;
            Stream >> Val.PrereqName;
            Stream >> Val.PrereqPath;
            Stream >> Val.PrereqArgs;
        }

        if (DataVersion >= ManifestMetaVersion::SerializesBuildId) {
            Stream >> Val.BuildId;
        }
        else {
            Val.BuildId = Val.GetBackwardsCompatibleBuildId();
        }

        Stream.seek(StartPos + DataSize, UEStream::Beg);
        return Stream;
    }

    std::string ManifestMeta::GetBackwardsCompatibleBuildId() const {
        char Hash[20];
        Utils::SHA1Builder Builder;
        Builder.Update((char*)&AppId, sizeof(AppId));
        Builder.Update((char*)&AppName, sizeof(AppName));
        Builder.Update((char*)&BuildVersion, sizeof(BuildVersion));
        Builder.Update((char*)&LaunchExe, sizeof(LaunchExe));
        Builder.Update((char*)&LaunchCommand, sizeof(LaunchCommand));
        Builder.Finish(Hash);

        if (!EGL3_ENSURE(!Builder.HasError(), LogLevel::Error, "Could not get compatible build id (SHA error)")) {
            return "";
        }

        auto Ret = Utils::B64Encode((uint8_t*)Hash, sizeof(Hash));
        std::replace(Ret.begin(), Ret.end(), '+', '-');
        std::replace(Ret.begin(), Ret.end(), '/', '_');
        auto EndPos = Ret.find_last_not_of('=');
        if (EndPos != std::string::npos) {
            Ret.erase(Ret.begin() + EndPos + 1, Ret.end());
        }

        return Ret;
    }
}