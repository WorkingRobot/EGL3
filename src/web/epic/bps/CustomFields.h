#pragma once

#include "ChunkDataList.h"
#include "UEStream.h"

namespace EGL3::Web::Epic::BPS {
    struct CustomFields {
        std::unordered_map<std::string, std::string> Fields;

        friend UEStream& operator>>(UEStream& Stream, CustomFields& Val);
    };
}