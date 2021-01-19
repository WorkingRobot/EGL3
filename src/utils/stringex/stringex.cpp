#include "stringex.h"

#include "expression_tree.h"

bool StringEx::Evaluate(std::string&& input, std::string&& expression)
{
    if (expression == "*") {
        return true;
    }

    if (expression.starts_with("Regex(")) {
        expression.insert(6, "\"");
        int parScope = 0;
        size_t boolIdx = 0;
        for (auto n = expression.begin() + 5; n != expression.end(); ++n) {
            if (*n == '(') {
                parScope++;
            }
            else if (*n == ')') {
                parScope--;
                if (!parScope) {
                    boolIdx = n - expression.begin();
                    expression.insert(boolIdx, "\"");
                    break;
                }
            }
        }/*
        auto andIdx = boolIdx;
        while (true) {
            andIdx = expression.find("&&", andIdx);
            if (andIdx == std::string::npos) break;

            expression.replace(andIdx, 2, "&");

            andIdx += 2;
        }
        auto orIdx = boolIdx;
        while (true) {
            orIdx = expression.find("||", orIdx);
            if (orIdx == std::string::npos) break;

            expression.replace(orIdx, 2, "|");

            orIdx += 2;
        }
        auto eqIdx = boolIdx;
        while (true) {
            eqIdx = expression.find("==", eqIdx);
            if (eqIdx == std::string::npos) break;

            expression.replace(eqIdx, 2, "=");

            eqIdx += 2;
        }*/
    }

    ExpressionTree tree;
    tree.SetInfixExpression(expression.c_str());
    tree.SetUserFunctionInput(std::move(input));
    return tree.EvaluateExpression();
}