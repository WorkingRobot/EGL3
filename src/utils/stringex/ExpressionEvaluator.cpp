#include "ExpressionEvaluator.h"

#include "ExpressionCompiler.h"
#include "Operators.h"

#include <regex>
#include <stack>

namespace EGL3::Utils::StringEx {
    ExpressionEvaluator::ExpressionEvaluator()
    {
        using namespace Operators;

        {
            TokenDefinitions.SetIgnoreWhitespace();
            TokenDefinitions.DefineToken(&ConsumeBool);
            TokenDefinitions.DefineToken(&ConsumeNumber);
            TokenDefinitions.DefineToken(&ConsumeString);
            TokenDefinitions.DefineToken(&ConsumeOperator<SubExpressionStart>);
            TokenDefinitions.DefineToken(&ConsumeOperator<SubExpressionEnd>);
            TokenDefinitions.DefineToken(&ConsumeOperator<LessOrEqual>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Less>);
            TokenDefinitions.DefineToken(&ConsumeOperator<GreaterOrEqual>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Greater>);
            TokenDefinitions.DefineToken(&ConsumeOperator<NotEqual>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Equal>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Or>);
            TokenDefinitions.DefineToken(&ConsumeOperator<And>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Not>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Plus>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Minus>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Multiply>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Divide>);
            TokenDefinitions.DefineToken(&ConsumeOperator<Modulo>);
            TokenDefinitions.DefineToken(&ConsumeFunction<FuncRegex>);
            TokenDefinitions.DefineToken(&ConsumeFunction<FuncRegexGroupInt64>);
            TokenDefinitions.DefineToken(&ConsumeFunction<FuncRegexGroupString>);
            TokenDefinitions.DefineToken([this](TokenStream& Stream, std::vector<ExpressionToken>& Tokens) { return ConsumeCustomFunction(Stream, Tokens, Functions); });
        }

        {
            Grammar.DefineGrouping<SubExpressionStart, SubExpressionEnd>();
            Grammar.DefineBinaryOperator<LessOrEqual>(1);
            Grammar.DefineBinaryOperator<Less>(1);
            Grammar.DefineBinaryOperator<GreaterOrEqual>(1);
            Grammar.DefineBinaryOperator<Greater>(1);
            Grammar.DefineBinaryOperator<NotEqual>(1);
            Grammar.DefineBinaryOperator<Equal>(1);
            Grammar.DefineBinaryOperator<Or>(2);
            Grammar.DefineBinaryOperator<And>(2);
            Grammar.DefineBinaryOperator<Plus>(5);
            Grammar.DefineBinaryOperator<Minus>(5);
            Grammar.DefineBinaryOperator<Multiply>(4);
            Grammar.DefineBinaryOperator<Divide>(4);
            Grammar.DefineBinaryOperator<Modulo>(4);
            Grammar.DefinePreUnaryOperator<Not>();
            Grammar.DefinePreUnaryOperator<FuncRegex>();
            Grammar.DefinePreUnaryOperator<FuncRegexGroupInt64>();
            Grammar.DefinePreUnaryOperator<FuncRegexGroupString>();
            Grammar.DefinePreUnaryOperator<FuncCustom>();
        }

        {
            JumpTable.MapBinary<Plus, int64_t>();
            JumpTable.MapBinary<Minus, int64_t>();
            JumpTable.MapBinary<Multiply, int64_t>();
            JumpTable.MapBinaryFull<Divide, int64_t>();
            JumpTable.MapBinaryFull<Modulo, int64_t>();
            JumpTable.MapBinary<LessOrEqual, int64_t>();
            JumpTable.MapBinary<Less, int64_t>();
            JumpTable.MapBinary<GreaterOrEqual, int64_t>();
            JumpTable.MapBinary<Greater, int64_t>();
            JumpTable.MapBinary<NotEqual, int64_t>();
            JumpTable.MapBinary<Equal, int64_t>();
            JumpTable.MapBinary<NotEqual, std::string>();
            JumpTable.MapBinary<Equal, std::string>();
            JumpTable.MapBinary<Or, bool>();
            JumpTable.MapBinary<And, bool>();
            JumpTable.MapPreUnary<Not, bool>();

            JumpTable.MapPreUnaryFull<FuncRegex, std::string>();
            JumpTable.MapPreUnaryFull<FuncRegexGroupInt64, std::string>();
            JumpTable.MapPreUnaryFull<FuncRegexGroupString, std::string>();
            JumpTable.MapPreUnaryFull<FuncCustom, FuncCustomData>();
        }
    }

    void ExpressionEvaluator::AddFunction(const std::string& Name, const std::function<std::any(const std::string&)> Func)
    {
        Functions.emplace(Name, Func);
    }

    void ExpressionEvaluator::RemoveFunction(const std::string& Name)
    {
        Functions.erase(Name);
    }

    ExpressionError ExpressionEvaluator::Evaluate(const std::string& Expression, const std::string& Input, bool& Output) const
    {
        std::vector<ExpressionToken> LexedTokens;
        auto Error = TokenDefinitions.Lex(Expression, LexedTokens);
        if (Error.HasError()) {
            return Error;
        }

        std::vector<CompiledToken> CompiledTokens;
        Error = ExpressionCompiler(Grammar, LexedTokens, CompiledTokens);
        if (Error.HasError()) {
            return Error;
        }

        std::any Result;
        Error = Evaluate(CompiledTokens, ExpressionContext(Expression, Input, Functions), Result);
        if (Error.HasError()) {
            return Error;
        }

        auto TryOutput = std::any_cast<bool>(&Result);
        if (TryOutput) {
            Output = *TryOutput;
            return ExpressionError();
        }
        else {
            return ExpressionError("Output is not a bool");
        }
    }

    bool ExpressionEvaluator::Evaluate(const std::string& Expression, const std::string& Input) const
    {
        bool Output;
        if (Evaluate(Expression, Input, Output).HasError()) {
            return false;
        }
        return Output;
    }

    ExpressionError ExpressionEvaluator::Evaluate(const std::vector<CompiledToken>& CompiledTokens, const ExpressionContext& Ctx, std::any& Output) const
    {
        std::vector<ExpressionToken> RuntimeGeneratedTokens;
        std::stack<size_t> OperandStack;

        auto GetToken = [&](size_t Index) -> const ExpressionToken& {
            if (Index < CompiledTokens.size())
            {
                return CompiledTokens[Index];
            }

            return RuntimeGeneratedTokens[Index - CompiledTokens.size()];
        };

        auto AddToken = [&](ExpressionToken&& In) -> size_t {
            auto Index = CompiledTokens.size() + RuntimeGeneratedTokens.size();
            RuntimeGeneratedTokens.emplace_back(std::move(In));
            return Index;
        };

        for (int i = 0; i < CompiledTokens.size(); ++i) {
            auto& Token = CompiledTokens[i];

            switch (Token.Type)
            {
            case TokenType::Benign:
                continue;
            case TokenType::Operand:
                OperandStack.emplace(i);
                continue;
            case TokenType::ShortCircuit:
                if (!OperandStack.empty() && Token.ShortCircuitIdx == -1 && JumpTable.ShouldShortCircuit(Token, GetToken(OperandStack.top()), &Ctx))
                {
                    i = Token.ShortCircuitIdx;
                }
                continue;
            case TokenType::BinaryOperator:
                if (OperandStack.size() >= 2)
                {
                    auto R = GetToken(OperandStack.top());
                    OperandStack.pop();
                    auto L = GetToken(OperandStack.top());
                    OperandStack.pop();

                    std::any OpResult;
                    ExpressionError OpError = JumpTable.ExecBinary(Token, L, R, &Ctx, OpResult);
                    if (OpError.HasError())
                    {
                        return OpError;
                    }
                    else
                    {
                        OperandStack.emplace(AddToken(ExpressionToken(L.Token, std::move(OpResult))));
                    }
                }
                else
                {
                    return ExpressionError("Not enough operands for binary operator");
                }
                break;
            case TokenType::PostUnaryOperator:
            case TokenType::PreUnaryOperator:
                if (OperandStack.size() >= 1)
                {
                    auto Operand = GetToken(OperandStack.top());
                    OperandStack.pop();

                    std::any OpResult;
                    ExpressionError OpError = (Token.Type == TokenType::PreUnaryOperator) ?
                        JumpTable.ExecPreUnary(Token, Operand, &Ctx, OpResult) :
                        JumpTable.ExecPostUnary(Token, Operand, &Ctx, OpResult);

                    if (OpError.HasError())
                    {
                        return OpError;
                    }
                    else
                    {
                        OperandStack.emplace(AddToken(ExpressionToken(Operand.Token, std::move(OpResult))));
                    }
                }
                else
                {
                    return ExpressionError("No operand for unary operator");
                }
                break;
            }
        }

        if (OperandStack.size() == 1) {
            Output = GetToken(OperandStack.top()).Node;
            return ExpressionError();
        }

        return ExpressionError("Could not evaluate expression");
    }
}