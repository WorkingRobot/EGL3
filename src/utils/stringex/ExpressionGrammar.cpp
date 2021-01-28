#include "ExpressionGrammar.h"

namespace EGL3::Utils::StringEx {
    const std::type_index* ExpressionGrammar::GetGrouping(const std::type_index& Type) const
    {
        auto Itr = Groupings.find(Type);
        if (Itr != Groupings.end()) {
            return &Itr->second;
        }
        return nullptr;
    }

    bool ExpressionGrammar::HasPreUnaryOperator(const std::type_index& Type) const
    {
        return PreUnaryOperators.contains(Type);
    }

    bool ExpressionGrammar::HasPostUnaryOperator(const std::type_index& Type) const
    {
        return PostUnaryOperators.contains(Type);
    }

    const OpParameters* ExpressionGrammar::GetBinaryOperatorDefParameters(const std::type_index& Type) const
    {
        auto Itr = BinaryOperators.find(Type);
        if (Itr != BinaryOperators.end()) {
            return &Itr->second;
        }
        return nullptr;
    }
}