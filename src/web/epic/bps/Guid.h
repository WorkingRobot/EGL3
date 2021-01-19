#pragma once

#include "UEStream.h"

#include <compare>

namespace EGL3::Web::Epic::BPS {
    struct Guid {
        uint32_t A;
        uint32_t B;
        uint32_t C;
        uint32_t D;

        Guid();

        friend UEStream& operator>>(UEStream& Stream, Guid& Val);

        /*std::strong_ordering operator<=>(const Guid& that) const {
            return 0 <=> memcmp(Data, that.Data, 16);
        }*/

    };

    bool operator==(const Guid& lhs, const Guid& rhs);
}

namespace std {
    template<> struct hash<EGL3::Web::Epic::BPS::Guid>
    {
        std::size_t operator()(EGL3::Web::Epic::BPS::Guid const& s) const noexcept;
    };
}