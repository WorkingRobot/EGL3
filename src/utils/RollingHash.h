#pragma once

#include <string>

namespace EGL3::Utils {
    // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/BuildPatchServices/Private/BuildPatchHash.h
    // Specifically GetHashForDataSet, used for verifying chunks: https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/BuildPatchServices/Private/Data/ChunkData.cpp#L675
    namespace Detail {
        template <uint64_t c, int k = 8>
        struct fH : fH<(c >> 1) ^ (0xC96C5795D7870F42 & (-(c & 1))), k - 1> {};
        template <uint64_t c> struct fH<c, 0> { enum n : uint64_t { value = c }; }; // without "n : uint64_t" msvc screws up

#define A(x) B(x) B(x + 128)
#define B(x) C(x) C(x +  64)
#define C(x) D(x) D(x +  32)
#define D(x) E(x) E(x +  16)
#define E(x) F(x) F(x +   8)
#define F(x) G(x) G(x +   4)
#define G(x) H(x) H(x +   2)
#define H(x) I(x) I(x +   1)
#define I(x) fH<x>::value,

        static constexpr uint64_t rh_table[] = { A(0) };

#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef H
#undef I

        static constexpr uint64_t rh_impl_one(uint8_t d, uint64_t crc) {
            return (crc << 1 | crc >> 63) ^ rh_table[d];
        }

        static constexpr uint64_t rh_impl(const char* p, size_t len, uint64_t crc) {
            return len ? rh_impl(p + 1, len - 1, rh_impl_one(*(uint8_t*)p, crc)) : crc;
        }
    }

    static constexpr __forceinline uint64_t RollingHash(const char* str, size_t size) {
        const uint8_t* Buffer = (uint8_t*)str;
        uint64_t Hash = 0;
        for (size_t i = 0; i < size; ++i) {
            Hash = Detail::rh_impl_one(Buffer[i], Hash);
        }
        return Hash;
    }

    static __forceinline uint64_t RollingHash(const std::string& str) {
        return RollingHash(str.c_str(), str.size());
    }

    template<size_t size>
    static constexpr __forceinline uint64_t RollingHash(const char(&str)[size]) {
        return RollingHash(str, size - 1);
    }
}
