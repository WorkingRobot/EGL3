#pragma once

#include "ManifestHeader.h"
#include "ManifestMeta.h"
#include "ChunkDataList.h"
#include "FileManifestList.h"
#include "CustomFields.h"
#include "../../../utils/streams/Stream.h"

#include <rapidjson/document.h>

namespace EGL3::Web::Epic::BPS {
    class Manifest {
    public:
        Manifest(const char* Data, size_t DataSize);

        Manifest(const rapidjson::Document& Json);

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, Manifest& Val);

        enum class ErrorType : uint8_t {
            Success,
            InvalidMagic,
            InvalidJson,
            BadJson,
            TooOld,
            BadCompression,
            BadHash,
        };

        bool HasError() const;

        ErrorType GetError() const;

        const FileManifest* GetFile(const std::string& Filename) const;

        const ChunkInfo* GetChunk(const Utils::Guid& Guid) const;

    private:
        void ReadFromJson(const rapidjson::Document& Json);

        void SetError(ErrorType NewError);

        ErrorType Error;

    public:
        ManifestMeta ManifestMeta;
        ChunkDataList ChunkDataList;
        FileManifestList FileManifestList;
        CustomFields CustomFields;
    };
}