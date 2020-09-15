#pragma once

#include "expression_tree_common.h"

#include <regex>

class CUserFunctions
{
private:
    std::string regInput;
    std::smatch regMatches;

    MAP_USER_FUNC mapFuncDefinitions;
    bool Initialize();
    void InsertMap(int nRtnType, int nRtnTypeDetailed, const char* strFuncName, int nArgCnt, ...);

protected:
    char szReturn[1024];

public:
    CUserFunctions();
    ~CUserFunctions();

    void SetInput(std::string&& inp);
    bool RunUserFuncRegex(const char* inp);
    std::string RunUserFuncRegexGroupString(long inp);
    int64_t RunUserFuncRegexGroupInt64(long inp);

    bool GetStringDoubleQuotationsBothSidesRemoved(char* strExpression);
    bool IsThisUserFunction(const char* strFuncName, IT_MAP_USER_FUNC& itFound);

    bool InvokeUserFunction(expression_node* pRoot, expression_node* pFuncArgLeft, expression_node* pFuncArgRight);
};