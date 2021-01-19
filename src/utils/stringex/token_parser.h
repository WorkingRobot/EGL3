#pragma once

#include "expression_tree_common.h"
#include "user_functions.h"

#include <stack>

typedef struct __ItemTokenInfo
{
    int     nType;
    int     nDetailedType;
    int     nLength;
    char    strValue[1024];
    bool    bThisTokenBelongsToTheUserFunctions; //true only if this token belongs to a function
    SFuncDefInfo userFuncInfo;
} ItemTokenInfo;

enum EnumErrorTokenType
{
    TKN_ERROR = 0, // Error Token -> Incorrect condition
    TKN_UNKNOWN
};

enum EnumNodeType
{
    NODE_UNKNOWN = 0,
    NODE_LITERAL,            // " or A-z or . or _
    NODE_NUMBER,             // Integer Value
    NODE_BOOL,               // true/false
    NODE_COMPARE_OPERATOR,   // Operator  =, <, >, !, &, |
    NODE_CALCULATE_OPERATOR, // Operator  +, -, *, /
    NODE_DELIMITER,          // Delimiter (, ), ,
    NODE_USER_FUNCTION,     // functions in CUserFunctions
    NODE_EMPTY,              // empty args
    MAX_NODE_TYPE
};

enum EnumNodeDetailedType
{
    //TKN_CALCULAT_OPERATOR
    OPERATOR_PLUS,          // +
    OPERATOR_MINUS,         // -
    OPERATOR_TIMES,         // *
    OPERATOR_DIVIDE,        // /

    //TKN_COMPARE_OPERATOR
    COMPARE_AND,            // &
    COMPARE_OR,             // |
    COMPARE_EQUALS,         // =
    COMPARE_NOT_EQUALS,     // !=
    COMPARE_GREATER_THAN,   // >
    COMPARE_GREATER_EQUALS, // >=
    COMPARE_LESS_THAN,      // <
    COMPARE_LESS_EQUALS,    // <=

    //TKN_DELIMITER
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    COMMA,
    USER_FUNCTIION,
    LITERAL,
    NUMBER_INT,
    NUMBER_LONG,
    NUMBER_FLOAT,
    BOOL_TYPE
};

enum EnumTypeOfOneCharToken
{
    CHAR_LITERAL = 300, // Character is Literal
    CHAR_NUMBER         // Character is Number
};

constexpr char CHAR_DOUBLE_QUOTATION    = '"';
constexpr char CHAR_LEFT_PARENTHESIS    = '(';
constexpr char CHAR_RIGHT_PARENTHESIS   = ')';
constexpr char CHAR_MINUS               = '-';
constexpr char CHAR_TIMES               = '*';
constexpr char CHAR_PLUS                = '+';
constexpr char CHAR_DEVIDE              = '/';
constexpr char CHAR_EQUALS              = '=';
constexpr char CHAR_LESS_THAN           = '<';
constexpr char CHAR_GREATER_THAN        = '>';
constexpr char CHAR_NOT                 = '!';
constexpr char CHAR_AND                 = '&';
constexpr char CHAR_OR                  = '|';
constexpr char CHAR_SPACE               = ' ';
constexpr char CHAR_SEMICOLON           = ';';
constexpr char CHAR_COLON               = ':';
constexpr char CHAR_QUESTION            = '?';
constexpr char CHAR_PERCENT             = '%';
constexpr char CHAR_DOLLAR              = '$';
constexpr char CHAR_COMMA               = ',';

class CTokenParser
{
private:
    char* strExpression;
    int GetTokenType(char chexp);
    int GetNumberCount(char* strexp);
    int GetLiteralCount(char* strexp);
    int nFuncDepth;
    std::stack<SFuncDefInfo> processingFunctionStack;
    bool bIsAvailableNegativePositiveNumberNowForThePreviousType; //for checking negative number
    bool bThisIsNegativeNumber;
    bool bThisIsPositiveNumber;

protected:
    int GetIdentifierCount(char* strexp);
    int GetTokenType(char*);
    CUserFunctions funcMap;

public:
    CTokenParser();
    ~CTokenParser();

    void PutExpression(char* exp);
    bool GetToken(ItemTokenInfo* pResult);
};