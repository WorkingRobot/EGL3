#pragma once

#include <type_traits>

namespace EGL3::Utils {
    template <typename... Ts>
    static constexpr __forceinline size_t HashCombine(Ts... Rest) {
        size_t Seed = 0;
        hash_combine(Seed, Rest...);
        return Seed;
    }

    namespace {
        static constexpr __forceinline void hash_combine(std::size_t& seed) { }

        // https://stackoverflow.com/a/38140932
        template <typename T, typename... Rest>
        static constexpr __forceinline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
            seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
            hash_combine(seed, rest...);
        }
    }
}