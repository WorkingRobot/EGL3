#pragma once

#include "CompiledToken.h"
#include "ExpressionError.h"
#include "ExpressionGrammar.h"
#include "ExpressionToken.h"

#include <functional>
#include <vector>

namespace EGL3::Utils::StringEx {
    class ExpressionCompiler {
    public:
        ExpressionCompiler(const ExpressionGrammar& Grammar, const std::vector<ExpressionToken>& Input, std::vector<CompiledToken>& Output);

        operator ExpressionError() const {
            return Error;
        }

    private:
        ExpressionError CompileGroup(const ExpressionToken* Start, const std::type_index* StopAt);

        ExpressionError Error;
        std::vector<ExpressionToken>::const_iterator Itr;

        const ExpressionGrammar& Grammar;
        const std::vector<ExpressionToken>& Input;
        std::vector<CompiledToken>& Output;
    };
}