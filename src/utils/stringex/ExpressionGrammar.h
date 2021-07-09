#pragma once

#include "../Log.h"
#include "OpParameters.h"

#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace EGL3::Utils::StringEx {
    class ExpressionGrammar {
    public:
        const std::type_index* GetGrouping(const std::type_index& Type) const;

        bool HasPreUnaryOperator(const std::type_index& Type) const;

        bool HasPostUnaryOperator(const std::type_index& Type) const;

        const OpParameters* GetBinaryOperatorDefParameters(const std::type_index& Type) const;

        template<typename TStartGroup, typename TEndGroup>
        void DefineGrouping() {
            Groupings.emplace(typeid(TStartGroup), typeid(TEndGroup));
        }

        template<typename TExpressionNode>
        void DefinePreUnaryOperator() {
            PreUnaryOperators.emplace(typeid(TExpressionNode));
        }

        template<typename TExpressionNode>
        void DefinePostUnaryOperator() {
            PreUnaryOperators.emplace(typeid(TExpressionNode));
        }

        template<typename TExpressionNode>
        void DefineBinaryOperator(size_t Precedence, Associativity Associativity = Associativity::RightToLeft, bool CanShortCircuit = false) {
            if constexpr (false) { // runtime check
                for (auto& Val : BinaryOperators) {
                    if (Val.second.Precedence == Precedence) {
                        // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Core/Public/Misc/ExpressionParserTypes.h#L553
                        EGL3_VERIFY(Val.second.Associativity == Associativity, "Operators of the same precedence must all have the same associativity");
                    }
                }
            }

            BinaryOperators.emplace(typeid(TExpressionNode), OpParameters(Precedence, Associativity, CanShortCircuit));
        }

    private:
        std::unordered_map<std::type_index, std::type_index> Groupings;
        std::unordered_set<std::type_index> PreUnaryOperators;
        std::unordered_set<std::type_index> PostUnaryOperators;
        std::unordered_map<std::type_index, OpParameters> BinaryOperators;
    };
}