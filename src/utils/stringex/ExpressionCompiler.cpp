#include "ExpressionCompiler.h"

#include <stack>

namespace EGL3::Utils::StringEx {
    ExpressionCompiler::ExpressionCompiler(const ExpressionGrammar& Grammar, const std::vector<ExpressionToken>& Input, std::vector<CompiledToken>& Output) :
        Grammar(Grammar),
        Input(Input),
        Output(Output),
        Itr(Input.begin())
    {
        Output.clear();

        CompileGroup(nullptr, nullptr);
    }

    struct WrappedOperator {
        WrappedOperator(CompiledToken&& Token, int Precedence = 0, int ShortCircuitIdx = -1) :
            Token(std::move(Token)),
            Precedence(Precedence),
            ShortCircuitIdx(ShortCircuitIdx)
        {

        }

        CompiledToken Token;
        int Precedence;
        int ShortCircuitIdx;
    };

    ExpressionError ExpressionCompiler::CompileGroup(const ExpressionToken* Start, const std::type_index* StopAt)
    {
        enum class State : uint8_t {
            PreUnary,
            PostUnary,
            Binary
        };

        std::stack<WrappedOperator> OperatorStack;
        
        auto PopOperator = [&] {
            auto ShortCircuitIdx = OperatorStack.top().ShortCircuitIdx;
            Output.emplace_back(std::move(OperatorStack.top().Token));
            OperatorStack.pop();
            if (ShortCircuitIdx != -1) {
                Output[ShortCircuitIdx].ShortCircuitIdx = Output.size() - 1;
            }
        };

        bool AtEnd = StopAt == nullptr;

        State CurrentState = State::PreUnary;
        for (; Itr != Input.end(); ++Itr) {
            auto& TypeId = Itr->Node.type();

            if (auto GroupingEnd = Grammar.GetGrouping(TypeId)) {
                auto NextBeginItr = Itr;

                ++Itr;

                auto Error = CompileGroup(&*NextBeginItr, GroupingEnd);

                if (Error.HasError()) {
                    return Error;
                }

                CurrentState = State::PostUnary;
            }
            else if (StopAt && TypeId == *StopAt) {
                AtEnd = true;
                break;
            }
            else if (CurrentState == State::PreUnary) {
                if (Grammar.HasPreUnaryOperator(TypeId)) {
                    OperatorStack.emplace(CompiledToken(TokenType::PreUnaryOperator, *Itr));
                }
                else if (Grammar.GetBinaryOperatorDefParameters(TypeId)) {
                    return ExpressionError("No operand specified for such operator");
                }
                else if (Grammar.HasPostUnaryOperator(TypeId)) {
                    CurrentState = State::PostUnary;

                    while (!OperatorStack.empty() && OperatorStack.top().Precedence <= 0) {
                        PopOperator();
                    }

                    OperatorStack.emplace(CompiledToken(TokenType::PostUnaryOperator, *Itr));
                }
                else {
                    OperatorStack.emplace(CompiledToken(TokenType::Operand, *Itr));
                    CurrentState = State::PostUnary;
                }
            }
            else if (CurrentState == State::PostUnary) {
                if (Grammar.HasPostUnaryOperator(TypeId)) {
                    while (!OperatorStack.empty() && OperatorStack.top().Precedence <= 0) {
                        PopOperator();
                    }

                    OperatorStack.emplace(CompiledToken(TokenType::PostUnaryOperator, *Itr));
                }
                else {
                    if (auto Params = Grammar.GetBinaryOperatorDefParameters(TypeId)) {
                        auto CheckPrecedence = [Params](int LastPrec, int Prec) {
                            return (Params->Associativity == Associativity::LeftToRight ? (LastPrec <= Prec) : (LastPrec < Prec));
                        };

                        while (!OperatorStack.empty() && CheckPrecedence(OperatorStack.top().Precedence, Params->Precedence)) {
                            PopOperator();
                        }

                        int ShortCircuitIdx = -1;
                        if (Params->CanShortCircuit) {
                            OperatorStack.emplace(CompiledToken(TokenType::ShortCircuit, *Itr), Output.size());
                            ShortCircuitIdx = Output.size() - 1;
                        }

                        OperatorStack.emplace(CompiledToken(TokenType::BinaryOperator, *Itr), Params->Precedence, ShortCircuitIdx);

                        CurrentState = State::PreUnary;
                    }
                    else {
                        OperatorStack.emplace(CompiledToken(TokenType::Operand, *Itr));
                        CurrentState = State::PreUnary;
                    }
                }
            }
        }

        if (!AtEnd) {
            return ExpressionError("Reached end of expresison before matching end of group");
        }

        while (!OperatorStack.empty()) {
            PopOperator();
        }

        return ExpressionError();
    }
}