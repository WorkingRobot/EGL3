#pragma once

#include <stdint.h>

namespace EGL3::Utils::StringEx {
    enum class Associativity : uint8_t {
        RightToLeft,
        LeftToRight
    };

    struct OpParameters {
        OpParameters(size_t Precedence, Associativity Associativity, bool CanShortCircuit) :
            Precedence(Precedence),
            Associativity(Associativity),
            CanShortCircuit(CanShortCircuit)
        {

        }

        size_t Precedence;
        Associativity Associativity;
        bool CanShortCircuit;
    };
}