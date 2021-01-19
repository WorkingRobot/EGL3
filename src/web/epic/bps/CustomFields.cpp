#include "CustomFields.h"

namespace EGL3::Web::Epic::BPS {
    UEStream& operator>>(UEStream& Stream, CustomFields& Val) {
        auto StartPos = Stream.tell();

        uint32_t DataSize;
        ChunkDataListVersion DataVersion;
        int32_t ElementCount;
        {
            Stream >> DataSize;
            uint8_t DataVersionRaw;
            Stream >> DataVersionRaw;
            DataVersion = (ChunkDataListVersion)DataVersionRaw;
            Stream >> ElementCount;
        }

        std::vector<std::pair<std::string, std::string>> FieldList;
        FieldList.resize(ElementCount);

        if (DataVersion >= ChunkDataListVersion::Original) {
            for (auto& Field : FieldList) { Stream >> Field.first; }
            for (auto& Field : FieldList) { Stream >> Field.second; }
        }

        Val.Fields.reserve(ElementCount);
        for (auto& Field : FieldList) {
            Val.Fields.emplace(Field.first, Field.second);
        }

        Stream.seek(StartPos + DataSize, UEStream::Beg);
        return Stream;
    }
}