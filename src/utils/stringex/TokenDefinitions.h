#pragma once

#include "ExpressionError.h"
#include "ExpressionToken.h"
#include "TokenStream.h"

#include <functional>
#include <vector>

namespace EGL3::Utils::StringEx {
    class TokenDefinitions {
    public:
        TokenDefinitions();

        bool GetIgnoreWhitespace() const;

        void SetIgnoreWhitespace(bool Value = true);
        
        void DefineToken(std::function<ExpressionError(TokenStream&, std::vector<ExpressionToken>&)>&& Definition);

        ExpressionError Lex(const std::string& Input, std::vector<ExpressionToken>& Output) const;

    private:
        ExpressionError LexOne(TokenStream& Stream, std::vector<ExpressionToken>& Output) const;

        bool IgnoreWhitespace;
        std::vector<std::function<ExpressionError(TokenStream&, std::vector<ExpressionToken>&)>> Definitions;
    };
}