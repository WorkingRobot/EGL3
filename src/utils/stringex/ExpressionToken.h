#pragma once

#include "StringToken.h"

#include <any>

namespace EGL3::Utils::StringEx {
    struct ExpressionToken {
        ExpressionToken(const StringToken& Token, std::any Node) :
            Token(Token),
            Node(std::move(Node))
        {

        }

        std::any Node;
        StringToken Token;
    };
}