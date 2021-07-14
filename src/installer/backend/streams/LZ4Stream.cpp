#include "LZ4Stream.h"

#include "../Constants.h"

#include <lz4hc.h>

namespace EGL3::Installer::Backend::Streams {
    LZ4Stream::LZ4Stream(Utils::Streams::Stream& BaseStream) :
        BaseStream(BaseStream),
        LZ4Ctx(LZ4_createStreamHC()),
        InputBuffer(std::make_unique<char[]>(BufferSize)),
        OutputBuffer(std::make_unique<char[]>(LZ4_COMPRESSBOUND(BufferSize))),
        BufPos(0)
    {
        LZ4_resetStreamHC_fast((LZ4_streamHC_t*)LZ4Ctx, LZ4HC_CLEVEL_MAX);
    }

    LZ4Stream::~LZ4Stream()
    {
        Compress(BufPos);

        LZ4_freeStreamHC((LZ4_streamHC_t*)LZ4Ctx);
    }

    Utils::Streams::Stream& LZ4Stream::write(const char* Buf, size_t BufCount)
    {
        while (BufCount) {
            auto BytesToUse = std::min(BufferSize - BufPos, BufCount);
            memcpy(InputBuffer.get() + BufPos, Buf, BytesToUse);
            BufCount -= BytesToUse;
            Buf += BytesToUse;
            BufPos += BytesToUse;
            if (BufPos == BufferSize) {
                if (!Compress(BufferSize)) {
                    break;
                }
                BufPos = 0;
            }
        }

        return *this;
    }

    bool LZ4Stream::Compress(int BufCount)
    {
        auto CompressedSize = LZ4_compress_HC_continue((LZ4_streamHC_t*)LZ4Ctx, InputBuffer.get(), OutputBuffer.get(), BufCount, LZ4_COMPRESSBOUND(BufferSize));
        if (CompressedSize <= 0) {
            return false;
        }
        BaseStream << CompressedSize;
        BaseStream << BufCount;
        BaseStream.write(OutputBuffer.get(), CompressedSize);
        return true;
    }
}
