#pragma once

#include <vector>
#include <stack>

#include "expression_tree_common.h"
#include "token_parser.h"

class ExpressionTree
{
public:
    ExpressionTree();
    virtual ~ExpressionTree();

    bool SetInfixExpression(const char* inFix);
    void SetUserFunctionInput(std::string&& input);
    bool EvaluateExpression();
    expression_result* GetResult();

protected:
    int nDepthOfTree;
    int memStatus;
    bool IsOperator(ItemTokenInfo* pNodeInfo);
    bool IsOperand(ItemTokenInfo* pNodeInfo);
    bool IsUserFunction(ItemTokenInfo* pNodeInfo);
    int GetOperatorPriority(ItemTokenInfo* pNodeInfo);
    int GetOperatorPrecedence(ItemTokenInfo* pNodeInfo1, ItemTokenInfo* pNodeInfo2);
    expression_node* CreateTreeNode();
    bool BuildExpressionTree();
    void DeleteExpressionTree(expression_node* root);
    bool CheckCompareError(expression_node* opRsltLeft, expression_node* opRsltRight);

    expression_node* root_node;
    std::vector<ItemTokenInfo> vecPostfixResult;
    CUserFunctions userFuncs;

    void EvaluateNumericCondition(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight);

    void EvaluateStringCondition(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight);

    void EvaluateBoolCondition(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight);

    void EvaluateConditionNonRecursive(expression_node* root);

    void EvaluateLogic(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight);

    std::stack<expression_node*> treeNodeMemRepository;
};