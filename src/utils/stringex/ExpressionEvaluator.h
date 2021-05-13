#pragma once

#include "CompiledToken.h"
#include "ExpressionError.h"
#include "ExpressionGrammar.h"
#include "JumpTable.h"
#include "TokenDefinitions.h"
#include "ExpressionContext.h"

#include <functional>
#include <string>
#include <unordered_map>

namespace EGL3::Utils::StringEx {
    // Based off of https://github.com/EpicGames/UnrealEngine/blob/master/Engine/Source/Runtime/Core/Public/Math/BasicMathExpressionEvaluator.h
    // That's the closest thing I could find realistically. There's no
    // way they coded a custom implementation from scratch just for EGL
    class ExpressionEvaluator {
    public:
        ExpressionEvaluator();

        void AddFunction(const std::string& Name, const std::function<std::any(const std::string&)> Func);

        void RemoveFunction(const std::string& Name);

        ExpressionError Evaluate(const std::string& Expression, const std::string& Input, bool& Output) const;

        bool Evaluate(const std::string& Expression, const std::string& Input) const;

    private:
        ExpressionError Evaluate(const std::vector<CompiledToken>& CompiledTokens, const ExpressionContext& Ctx, std::any& Output) const;

        TokenDefinitions TokenDefinitions;
        ExpressionGrammar Grammar;
        JumpTable<ExpressionContext> JumpTable;
        std::unordered_map<std::string, std::function<std::any(const std::string&)>> Functions;
    };
}