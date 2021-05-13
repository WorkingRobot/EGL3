#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace EGL3::Utils::StringEx {
    struct ExpressionContext {
        ExpressionContext(const std::string& Expression, const std::string& Input, const std::unordered_map<std::string, std::function<std::any(const std::string&)>>& Functions) :
            Expression(Expression),
            Input(Input),
            Functions(Functions)
        {

        }

        const std::string& Expression;
        const std::string& Input;
        const std::unordered_map<std::string, std::function<std::any(const std::string&)>>& Functions;
        mutable std::vector<std::string> CaptureGroups;
    };
}