#include "user_functions.h"

#include <iomanip>
#include <stdarg.h>

CUserFunctions::CUserFunctions()
{
    memset(&szReturn, 0x00, sizeof(szReturn));
    Initialize();
}

CUserFunctions::~CUserFunctions()
{

}

void CUserFunctions::InsertMap(int nRtnType, int nRtnTypeDetailed, const char* strFuncName, int nArgCnt, ...)
{
    SFuncDefInfo funcDef;
    strcpy(funcDef.strFuncName, strFuncName);
    funcDef.nReturnType = nRtnType;
    funcDef.nDetailedReturnType = nRtnTypeDetailed;
    funcDef.ntotalInputArgCnt = nArgCnt;

    va_list arguments;
    va_start(arguments, nArgCnt);
    for (int x = 0; x < nArgCnt; x++)
    {
        int nVar = va_arg(arguments, int);
        funcDef.nFuncArgsTypes[x] = nVar;
    }
    va_end(arguments);
    mapFuncDefinitions.insert(VT_MAP_USER_FUNC(strFuncName, funcDef));
}

bool CUserFunctions::Initialize()
{
    InsertMap(FUNC_RTN_BOOL, FUNC_RTN_DETAILED_BOOL, "Regex", 1, FUNC_ARG_STR);
    InsertMap(FUNC_RTN_STR, FUNC_RTN_DETAILED_STR, "RegexGroupString", 1, FUNC_ARG_NUM);
    InsertMap(FUNC_RTN_NUM, FUNC_RTN_DETAILED_LONG, "RegexGroupInt64", 1, FUNC_ARG_NUM);

    return true;
}

bool CUserFunctions::IsThisUserFunction(const char* strFuncName, IT_MAP_USER_FUNC& itFound)
{
    itFound = mapFuncDefinitions.find(strFuncName);
    return itFound != mapFuncDefinitions.end();
}

bool CUserFunctions::GetStringDoubleQuotationsBothSidesRemoved(char* strExpression)
{
    char* quotPtr = strchr(strExpression, '"');
    if (quotPtr == NULL)
    {
        return false;
    }

    int position = quotPtr - strExpression;
    char attrValue[1024];
    memset(&attrValue, 0x00, sizeof(attrValue));
    strncpy(attrValue, strExpression + position + 1, strlen(strExpression) - position);

    quotPtr = strchr(attrValue, '"');
    if (quotPtr == NULL)
    {
        return false;
    }
    position = quotPtr - attrValue;
    memset(strExpression, 0x00, sizeof(attrValue));
    strncpy(strExpression, attrValue, position);

    return true;
}

void CUserFunctions::SetInput(std::string&& inp)
{
    regInput = inp;
}

bool CUserFunctions::RunUserFuncRegex(const char* inp) {
    auto n = std::quoted(inp)._Ptr;
    return std::regex_match(regInput, regMatches, std::regex(n));
}

std::string CUserFunctions::RunUserFuncRegexGroupString(long inp) {
    return regMatches[inp].str();
}

int64_t CUserFunctions::RunUserFuncRegexGroupInt64(long inp) {
    return std::atoll(regMatches[inp].str().c_str());
}

bool CUserFunctions::InvokeUserFunction(expression_node* pRoot, expression_node* pFuncArgLeft, expression_node* pFuncArgRight)
{
    if (strcmp(pRoot->userFuncInfo.strFuncName, "Regex") == 0)
    {
        pRoot->expressionResult.bResult = RunUserFuncRegex(pFuncArgLeft->expressionResult.strResult);
    }
    else if (strcmp(pRoot->userFuncInfo.strFuncName, "RegexGroupString") == 0)
    {
        strcpy(pRoot->expressionResult.strResult, RunUserFuncRegexGroupString(pFuncArgLeft->expressionResult.nResultLong).c_str());
    }
    else if (strcmp(pRoot->userFuncInfo.strFuncName, "RegexGroupInt64") == 0)
    {
        pRoot->expressionResult.nResultLong = RunUserFuncRegexGroupInt64(pFuncArgLeft->expressionResult.nResultLong);
    }

    return true;
}