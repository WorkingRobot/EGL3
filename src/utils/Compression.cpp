#include "Compression.h"

#include "Log.h"

#include <zlib.h>

namespace EGL3::Utils {
    bool Compressor<CompressionMethod::Zlib>::Decompress(char* Dst, size_t DstSize, const char* Src, size_t SrcSize)
    {
        z_stream Stream;
        Stream.zalloc = Z_NULL;
        Stream.zfree = Z_NULL;
        Stream.opaque = Z_NULL;
        Stream.avail_in = (uInt)SrcSize;
        Stream.next_in = (Bytef*)Src;
        Stream.avail_out = (uInt)DstSize;
        Stream.next_out = (Bytef*)Dst;

        auto Result = inflateInit(&Stream);

        if (Result != Z_OK) {
            return false;
        }

        Result = inflate(&Stream, Z_FINISH);
        if (Result == Z_STREAM_END) {
            EGL3_ENSURE(Stream.total_out == DstSize, LogLevel::Warning, "Zlib uncompressed size does not match passed DstSize");
        }

        auto EndResult = inflateEnd(&Stream);

        return Result >= Z_OK && EndResult == Z_OK;
    }
}
