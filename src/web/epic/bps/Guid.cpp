#include "Guid.h"

#include <compare>

namespace EGL3::Web::Epic::BPS {
    Guid::Guid() :
        A{}, B{}, C{}, D{}
    {

    }

    UEStream& operator>>(UEStream& Stream, Guid& Val) {
        Stream >> Val.A;
        Stream >> Val.B;
        Stream >> Val.C;
        Stream >> Val.D;

        return Stream;
    }

    bool operator==(const Guid& lhs, const Guid& rhs) { return memcmp(&lhs, &rhs, 16) == 0; }
}

namespace std {
    std::size_t hash<EGL3::Web::Epic::BPS::Guid>::operator()(EGL3::Web::Epic::BPS::Guid const& s) const noexcept
    {
        return *(uint64_t*)&s ^ *(uint64_t*)&s.C;
    }
}