#pragma once

#include <stdint.h>

namespace EGL3::Installer::Backend {
    static constexpr uint32_t FileMagic = 0x324B4644;
    static constexpr uint32_t BufferSize = 1024 * 128; // 128 kb

#define LZ4_MAX_INPUT_SIZE        0x7E000000   /* 2 113 929 216 bytes */
#define LZ4_COMPRESSBOUND(isize)  ((unsigned)(isize) > (unsigned)LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize)/255) + 16)

    static constexpr uint32_t OutBufferSize = LZ4_COMPRESSBOUND(BufferSize);

#undef LZ4_MAX_INPUT_SIZE
#undef LZ4_COMPRESSBOUND
}