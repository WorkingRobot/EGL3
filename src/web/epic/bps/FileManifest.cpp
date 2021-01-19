#include "FileManifest.h"

#include "../../../utils/StringCompare.h"

namespace EGL3::Web::Epic::BPS {
    std::weak_ordering FileManifest::operator<=>(const FileManifest& that) const {
        return Utils::CompareStringsSensitive(Filename, that.Filename);
    }
}