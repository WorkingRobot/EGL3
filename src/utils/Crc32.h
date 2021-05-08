#pragma once

#include <string>

namespace EGL3::Utils {
    namespace Detail {
        static constexpr uint32_t ByteSwap(uint32_t Val) {
            return (((Val) >> 24) + (((Val) >> 8) & 0xff00) + (((Val) << 8) & 0xff0000) + ((Val) << 24));
        }

        // https://stackoverflow.com/a/28801005
        template <uint32_t c, int k = 8>
        struct f : f<(c >> 1) ^ (0xEDB88320 & (-(c & 1))), k - 1> {};
        template <uint32_t c> struct f<c, 0> { enum { value = c }; };

        // http://ffmpeg.org/doxygen/4.1/crc_8c_source.html#l00357
        // Used for:
        // - https://github.com/EpicGames/UnrealEngine/blob/fd72145a2cb3a3cbe5581db4077e206f8b0d58f9/Engine/Source/Runtime/Online/BuildPatchServices/Private/BuildPatchManifest.cpp#L639
        // - https://github.com/EpicGames/UnrealEngine/blob/2bf1a5b83a7076a0fd275887b373f8ec9e99d431/Engine/Source/Runtime/Core/Private/Misc/Crc.cpp#L483
        template <uint32_t c, int k = 9>
        struct f2 : f2<(c << 1) ^ (0x04C11DB7 & (((int32_t)c) >> 31)), k - 1> {};
        template <uint32_t c> struct f2<c, 0> { enum { value = ByteSwap(c) }; };
        template <uint32_t c> struct f2<c, 9> : f2<c << 24, 8> {};

#define A(c,x) B(c,x) B(c,x + 128)
#define B(c,x) C(c,x) C(c,x +  64)
#define C(c,x) D(c,x) D(c,x +  32)
#define D(c,x) E(c,x) E(c,x +  16)
#define E(c,x) F(c,x) F(c,x +   8)
#define F(c,x) G(c,x) G(c,x +   4)
#define G(c,x) H(c,x) H(c,x +   2)
#define H(c,x) I(c,x) I(c,x +   1)
#define I(c,x) c<x>::value,

        // f2 is "deprecated" version
        static constexpr uint32_t crc_table[] = { A(f, 0) A(f2, 0) };

#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef H
#undef I

        template<bool insensitive, bool deprecated>
        static constexpr uint32_t crc32_impl(const char* p, size_t len, uint32_t crc) {
            return len ?
                crc32_impl<insensitive, deprecated>(p + 1, len - 1, (crc >> 8) ^ crc_table[(deprecated << 8) | (crc & 0xFF) ^ (!insensitive ? *p : (*p >= 'a' && *p <= 'z' ? *p - 0x20 : *p))])
                : crc;
        }
    }

    template<bool insensitive = false, bool deprecated = false>
    static constexpr __forceinline uint32_t Crc32(const char* str, size_t size) {
        return ~Detail::crc32_impl<insensitive, deprecated>(str, size, ~0);
    }

    template<bool insensitive = false, bool deprecated = false>
    static __forceinline uint32_t Crc32(const std::string& str) {
        return Crc32<insensitive, deprecated>(str.c_str(), str.size());
    }

    template<bool insensitive = false, bool deprecated = false, size_t size>
    static constexpr __forceinline uint32_t Crc32(const char(&str)[size]) {
        return Crc32<insensitive, deprecated>(str, size - 1);
    }
}