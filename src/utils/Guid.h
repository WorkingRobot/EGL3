#pragma once

#include <compare>
#include <stdint.h>

namespace EGL3::Utils {
    struct Guid {
        uint32_t A;
        uint32_t B;
        uint32_t C;
        uint32_t D;

        Guid() :
            A{}, B{}, C{}, D{}
        {

        }

        std::strong_ordering operator<=>(const Guid& that) const
        {
            if (auto cmp = A <=> that.A; cmp != 0)
                return cmp;
            if (auto cmp = B <=> that.B; cmp != 0)
                return cmp;
            if (auto cmp = C <=> that.C; cmp != 0)
                return cmp;
            return D <=> that.D;
        }

        bool operator==(const Guid&) const = default;
    };
}

namespace std {
    template<> struct hash<EGL3::Utils::Guid>
    {
        std::size_t operator()(EGL3::Utils::Guid const& s) const noexcept
        {
            return *(uint64_t*)&s ^ *(uint64_t*)&s.C;
        }
    };
}