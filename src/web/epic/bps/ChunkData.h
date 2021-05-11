#pragma once

#include "../../../utils/streams/Stream.h"
#include "ChunkHeader.h"

#include <memory>

namespace EGL3::Web::Epic::BPS {
    class ChunkData {
    public:
        ChunkData(Utils::Streams::Stream& Stream);

        ChunkData(const char* Data, size_t DataSize);

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, ChunkData& Val);

        enum class ErrorType : uint8_t {
            Success,
            InvalidMagic,
            CorruptHeader,
            MissingHashInfo,
            IncorrectFileSize,
            UnsupportedStorage,
            DecompressFailure,
            HashCheckFailed,
        };

        bool HasError() const;

        ErrorType GetError() const;

    private:
        void SetError(ErrorType NewError);

        ErrorType Error;

    public:
        ChunkHeader Header;
        std::unique_ptr<char[]> Data;
    };
}