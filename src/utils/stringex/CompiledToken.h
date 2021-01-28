#pragma once

#include "ExpressionToken.h"

#include <stdint.h>

namespace EGL3::Utils::StringEx {
    enum class TokenType : uint8_t {
        Operand,
        PreUnaryOperator,
        PostUnaryOperator,
        BinaryOperator,
        ShortCircuit,
        Benign
    };

    struct CompiledToken : ExpressionToken {
        CompiledToken(TokenType Type, const ExpressionToken& Token, int ShortCircuitIdx = -1) :
            ExpressionToken(Token),
            Type(Type),
            ShortCircuitIdx(ShortCircuitIdx)
        {

        }

        TokenType Type;
        int ShortCircuitIdx;
    };
}