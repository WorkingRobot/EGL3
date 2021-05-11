#include "ChunkData.h"

#include "../../../utils/streams/BufferStream.h"
#include "../../../utils/Compression.h"
#include "../../../utils/RollingHash.h"
#include "../../../utils/SHA.h"

namespace EGL3::Web::Epic::BPS {
    using namespace Utils::Streams;

    ChunkData::ChunkData(Stream& Stream) :
        Error(ErrorType::Success)
    {
        Stream >> *this;
    }

    ChunkData::ChunkData(const char* Data, size_t DataSize) :
        Error(ErrorType::Success)
    {
        // Don't worry, we won't be writing to it
        BufferStream Stream((char*)Data, DataSize);

        Stream >> *this;
    }

    bool ChunkData::HasError() const
    {
        return GetError() != ErrorType::Success;
    }

    ChunkData::ErrorType ChunkData::GetError() const
    {
        return Error;
    }

    void ChunkData::SetError(ErrorType NewError)
    {
        Error = NewError;
    }

    // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/BuildPatchServices/Private/Data/ChunkData.cpp#L596
    Stream& operator>>(Stream& Stream, ChunkData& Val)
    {
        auto StartPos = Stream.tell();

        // Read header
        Stream >> Val.Header;

        if (Val.Header.Magic != ChunkHeader::ExpectedMagic) {
            Val.SetError(ChunkData::ErrorType::InvalidMagic);
            return Stream;
        }

        if (!Val.Header.Guid.IsValid()) {
            Val.SetError(ChunkData::ErrorType::CorruptHeader);
            return Stream;
        }

        if (Val.Header.HashType == ChunkHashFlags::None) {
            Val.SetError(ChunkData::ErrorType::MissingHashInfo);
            return Stream;
        }

        if ((uint64_t)Val.Header.HeaderSize + Val.Header.DataSizeCompressed > Stream.size() - StartPos) {
            Val.SetError(ChunkData::ErrorType::IncorrectFileSize);
            return Stream;
        }

        if (Val.Header.IsEncrypted()) {
            Val.SetError(ChunkData::ErrorType::UnsupportedStorage);
            return Stream;
        }

        // Read data
        Val.Data = std::make_unique<char[]>(Val.Header.DataSizeCompressed);
        Stream.read(Val.Data.get(), Val.Header.DataSizeCompressed);

        // Decompress data if necessary
        if (Val.Header.IsCompressed()) {
            // Move the current data to a compressed buffer, and decompress it back to the old variable
            std::unique_ptr<char[]> CompressedData = std::move(Val.Data);
            Val.Data = std::make_unique<char[]>(Val.Header.DataSizeUncompressed);

            bool DecompSuccess = Utils::Compressor<Utils::CompressionMethod::Zlib>::Decompress(
                Val.Data.get(), Val.Header.DataSizeUncompressed,
                CompressedData.get(), Val.Header.DataSizeCompressed
            );

            if (!DecompSuccess) {
                Val.SetError(ChunkData::ErrorType::DecompressFailure);
                return Stream;
            }
        }

        // Verify data integrity
        if (Val.Header.UsesRollingHash() && !Val.Header.UsesSha1()) {
            if (Val.Header.RollingHash != Utils::RollingHash(Val.Data.get(), Val.Header.DataSizeUncompressed))
            {
                Val.SetError(ChunkData::ErrorType::HashCheckFailed);
                return Stream;
            }
        }
        if (Val.Header.UsesSha1()) {
            if (!Utils::SHA1Verify(Val.Data.get(), Val.Header.DataSizeUncompressed, Val.Header.SHAHash)) {
                Val.SetError(ChunkData::ErrorType::HashCheckFailed);
                return Stream;
            }
        }

        Val.SetError(ChunkData::ErrorType::Success);
        return Stream;
    }
}