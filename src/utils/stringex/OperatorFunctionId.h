#pragma once

#include "../HashCombine.h"

#include <compare>
#include <typeindex>

namespace EGL3::Utils::StringEx {
    struct OperatorFunctionId {
        std::type_index OperatorType;
        std::type_index LeftOperandType;
        std::type_index RightOperandType;

        static std::strong_ordering CompareTypes(const std::type_index& A, const std::type_index& B) {
            if (A > B) {
                return std::strong_ordering::greater;
            }
            else if (A < B) {
                return std::strong_ordering::less;
            }
            else {
                return std::strong_ordering::equal;
            }
        }

        std::strong_ordering operator<=>(const OperatorFunctionId& that) const {
            if (auto cmp = CompareTypes(OperatorType, that.OperatorType); cmp != 0)
                return cmp;
            if (auto cmp = CompareTypes(LeftOperandType, that.LeftOperandType); cmp != 0)
                return cmp;
            return CompareTypes(RightOperandType, that.RightOperandType);
        }

        bool operator==(const OperatorFunctionId& that) const {
            return std::is_eq(*this <=> that);
        }
    };
}

namespace std
{
    template<> struct hash<EGL3::Utils::StringEx::OperatorFunctionId>
    {
        std::size_t operator()(const EGL3::Utils::StringEx::OperatorFunctionId& s) const noexcept
        {
            return EGL3::Utils::HashCombine(
                std::hash<std::type_index>{}(s.OperatorType),
                std::hash<std::type_index>{}(s.LeftOperandType),
                std::hash<std::type_index>{}(s.RightOperandType)
            );
        }
    };
}