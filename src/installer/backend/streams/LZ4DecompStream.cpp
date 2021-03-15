#include "LZ4DecompStream.h"

#include "../Constants.h"

#include <lz4.h>

namespace EGL3::Installer::Backend::Streams {
    LZ4DecompStream::LZ4DecompStream(Utils::Streams::Stream& BaseStream) :
        BaseStream(BaseStream),
        LZ4Ctx(LZ4_createStreamDecode()),
        CompBuffer(std::make_unique<char[]>(OutBufferSize)),
        DecompBuffer(std::make_unique<char[]>(BufferSize)),
        DecompLeft(0),
        DecompPos(0)
    {

    }

    LZ4DecompStream::~LZ4DecompStream()
    {
        LZ4_freeStreamDecode((LZ4_streamDecode_t*)LZ4Ctx);
    }

    Utils::Streams::Stream& LZ4DecompStream::read(char* Buf, size_t BufCount)
    {
        while (BufCount) {
            if (DecompLeft == DecompPos) {
                int CompSize;
                int DecompSize;
                BaseStream >> CompSize;
                BaseStream >> DecompSize;
                BaseStream.read(CompBuffer.get(), CompSize);
                DecompLeft = LZ4_decompress_safe_continue((LZ4_streamDecode_t*)LZ4Ctx, CompBuffer.get(), DecompBuffer.get(), CompSize, DecompSize);
                if (DecompLeft <= 0) {
                    break;
                }
                DecompPos = 0;
            }

            auto BytesToUse = std::min((size_t)DecompLeft - DecompPos, BufCount);
            memcpy(Buf, DecompBuffer.get() + DecompPos, BytesToUse);
            BufCount -= BytesToUse;
            Buf += BytesToUse;
            DecompPos += BytesToUse;
        }

        return *this;
    }
}
