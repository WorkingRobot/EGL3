#pragma once

namespace EGL3::Utils {
    template<uint64_t Alignment, class N>
    static constexpr N Align(N Value) {
        return (N)(((uint64_t)Value + Alignment - 1) & ~(Alignment - 1));
    }
}
