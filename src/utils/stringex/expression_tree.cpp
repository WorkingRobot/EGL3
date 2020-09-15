#include "expression_tree.h"

#include "token_parser.h"

constexpr const char* NodeTypeDesc[] = {
	"NODE_UNKNOWN",
	"NODE_LITERAL",
	"NODE_NUMBER",
	"NODE_COMPARE_OPERATOR",
	"NODE_CALCULATE_OPERATOR",
	"NODE_DELIMITER",
	"NODE_USER_FUNCTIION",
	"NODE_PLACE_HOLDER",
	"NODE_EMPTY"
};

constexpr const char* DetailedTypeDesc[] = {
	"OPERATOR_PLUS",
	"OPERATOR_MINUS",
	"OPERATOR_TIMES",
	"OPERATOR_DEVIDE",
	"COMPARE_AND",
	"COMPARE_OR",
	"COMPARE_EQUALS",
	"COMPARE_NOT_EQUALS",
	"COMPARE_GREATER_THAN",
	"COMPARE_GREATER_EQUALS",
	"COMPARE_LESS_THAN",
	"COMPARE_LESS_EQUALS",
	"LEFT_PARENTHESIS",
	"RIGHT_PARENTHESIS",
	"PAUSE",
	"USER_FUNCTIION",
	"LITERAL",
	"NUMBER_INT",
	"NUMBER_LONG",
	"NUMBER_FLOAT",
	"BOOL_TYPE",
	"PLACE_HOLDER"
};

// Operator priorities

const int MAX_OP_PRIORITY_TABLE_CNT = 12;

typedef struct _struct_OpPriority
{
	int nOpType;
	int nPriority;
} type_struct_OpPriority;

type_struct_OpPriority OpPriorityTable[MAX_OP_PRIORITY_TABLE_CNT] =
{
	{ COMPARE_AND,              1 },    // '&'
	{ COMPARE_OR,               1 },    // '|'
	{ COMPARE_EQUALS,           2 },    // '='
	{ COMPARE_NOT_EQUALS,       2 },    // '!='
	{ COMPARE_GREATER_THAN,     2 },    // '>'
	{ COMPARE_GREATER_EQUALS,   2 },    // '>='
	{ COMPARE_LESS_THAN,        2 },    // '<'
	{ COMPARE_LESS_EQUALS,      2 },    // '<='

	{ OPERATOR_PLUS,            3 },    // +
	{ OPERATOR_MINUS,           3 },    // -
	{ OPERATOR_TIMES,           4 },    // *
	{ OPERATOR_DIVIDE,          4 }     // /
};

ExpressionTree::ExpressionTree()
{
	memStatus = 0;
	root_node = NULL;
	nDepthOfTree = 0;
}

ExpressionTree::~ExpressionTree()
{
	DeleteExpressionTree(root_node);
}

bool ExpressionTree::IsUserFunction(ItemTokenInfo* pNodeInfo)
{
	return pNodeInfo->nType == NODE_USER_FUNCTION;
}

bool ExpressionTree::IsOperator(ItemTokenInfo* pNodeInfo)
{
	return pNodeInfo->nType == NODE_COMPARE_OPERATOR || pNodeInfo->nType == NODE_CALCULATE_OPERATOR;
}

bool ExpressionTree::IsOperand(ItemTokenInfo* pNodeInfo)
{
	return !IsOperator(pNodeInfo) &&
		pNodeInfo->nDetailedType != LEFT_PARENTHESIS &&
		pNodeInfo->nDetailedType != RIGHT_PARENTHESIS &&
		pNodeInfo->nDetailedType != COMMA;
}

int ExpressionTree::GetOperatorPriority(ItemTokenInfo* pNodeInfo)
{
	for (int i = 0; i < MAX_OP_PRIORITY_TABLE_CNT; i++)
	{
		if (pNodeInfo->nDetailedType == OpPriorityTable[i].nOpType)
		{
			return OpPriorityTable[i].nPriority;
		}
	}
	return 0;
}

int ExpressionTree::GetOperatorPrecedence(ItemTokenInfo* pNodeInfo1, ItemTokenInfo* pNodeInfo2)
{
	auto nPriority1 = GetOperatorPriority(pNodeInfo1);
	auto nPriority2 = GetOperatorPriority(pNodeInfo2);

	if (nPriority1 > nPriority2)
	{
		return -1;
	}
	else if (nPriority1 < nPriority2)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool ExpressionTree::SetInfixExpression(const char* inFix)
{
	std::vector<ItemTokenInfo>().swap(vecPostfixResult);
	//vecPostfixResult.shrink_to_fit();

	CTokenParser parser;
	parser.PutExpression((char*)inFix);

	std::stack<ItemTokenInfo> opStack;
	std::string postFixString = "";
	bool bErrorOccurred = false;

	while (1)
	{
		ItemTokenInfo token;
		memset(&token, 0x00, sizeof(token));
		if (!parser.GetToken(&token))
		{
			bErrorOccurred = true;
			break;
		}

		if (0 == token.nLength)
		{
			break;
		}

		if (IsUserFunction(&token))
		{
			opStack.push(token); // no precedence between functions
		}
		else if (IsOperand(&token))
		{
			postFixString += " ";
			postFixString += token.strValue;
			vecPostfixResult.emplace_back(token);
		}
		else if (IsOperator(&token))
		{
			while (!opStack.empty() &&
				LEFT_PARENTHESIS != opStack.top().nDetailedType &&
				GetOperatorPrecedence(&opStack.top(), &token) <= 0
				)
			{
				postFixString += " ";
				postFixString += opStack.top().strValue;
				vecPostfixResult.emplace_back(opStack.top());
				opStack.pop();
			}
			opStack.push(token);
		}
		else if (COMMA == token.nDetailedType && token.bThisTokenBelongsToTheUserFunctions) // SumInt(1+1, 2)
		{
			while (!opStack.empty() &&
				LEFT_PARENTHESIS != opStack.top().nDetailedType
				)
			{
				postFixString += " ";
				postFixString += opStack.top().strValue;
				vecPostfixResult.emplace_back(opStack.top());
				opStack.pop();
			}
		}
		else if (LEFT_PARENTHESIS == token.nDetailedType)
		{
			opStack.push(token);
		}
		else if (RIGHT_PARENTHESIS == token.nDetailedType)
		{
			// pop till starting '('
			while (!opStack.empty())
			{
				if (LEFT_PARENTHESIS == opStack.top().nDetailedType)
				{
					if (token.bThisTokenBelongsToTheUserFunctions)
					{
						// pop till starting function name
						opStack.pop();

						if (opStack.top().nDetailedType == LEFT_PARENTHESIS)
						{
							// (SumInt(1,1))
							opStack.pop();
						}
						else
						{
							postFixString += " ";
							postFixString += opStack.top().strValue; // function name
							vecPostfixResult.emplace_back(opStack.top());
							opStack.pop();
						}

						break;
					}
					else
					{
						opStack.pop();
						break;
					}
				}
				postFixString += " ";
				postFixString += opStack.top().strValue;
				vecPostfixResult.emplace_back(opStack.top());

				opStack.pop();
			}
		}
	}

	if (bErrorOccurred)
	{
		return false;
	}

	while (!opStack.empty())
	{
		postFixString += " ";
		postFixString += opStack.top().strValue;

		vecPostfixResult.emplace_back(opStack.top());

		opStack.pop();
	}

	return BuildExpressionTree();
}

void ExpressionTree::SetUserFunctionInput(std::string&& input)
{
	userFuncs.SetInput(std::move(input));
}

expression_node* ExpressionTree::CreateTreeNode()
{
	expression_node* node = new expression_node;

	treeNodeMemRepository.push(node);
	++memStatus;
	return node;
}


bool ExpressionTree::CheckCompareError(expression_node* pOpRsltLeft, expression_node* pOpRsltRight)
{
	if (pOpRsltLeft->expressionResult.nResultType == pOpRsltRight->expressionResult.nResultType)
	{
		return true;
	}
	else if (pOpRsltLeft->expressionResult.nResultType != pOpRsltRight->expressionResult.nResultType)
	{
		return false;
	}
	else
	{
		if (pOpRsltLeft->expressionResult.nResultDetailedType != pOpRsltRight->expressionResult.nResultDetailedType)
		{
			return false;
		}

		return true;
	}
}

void ExpressionTree::EvaluateStringCondition(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight)
{
	root->expressionResult.bResult = false;

	switch (root->nDetailedType)
	{
	case OPERATOR_PLUS:
		strcat(root->expressionResult.strResult, pRsltLeft->expressionResult.strResult);
		strcat(root->expressionResult.strResult, pRsltRight->expressionResult.strResult);
		break;
	case COMPARE_EQUALS:
		if (strcmp(pRsltLeft->expressionResult.strResult, pRsltRight->expressionResult.strResult) == 0)
		{
			root->expressionResult.bResult = true;
		}
		break;

	case COMPARE_NOT_EQUALS:
		if (strcmp(pRsltLeft->expressionResult.strResult, pRsltRight->expressionResult.strResult) != 0)
		{
			root->expressionResult.bResult = true;
		}
		break;

	case COMPARE_GREATER_THAN:
		if (strcmp(pRsltLeft->expressionResult.strResult, pRsltRight->expressionResult.strResult) > 0)
		{
			root->expressionResult.bResult = true;
		}
		break;

	case COMPARE_GREATER_EQUALS:
		if (strcmp(pRsltLeft->expressionResult.strResult, pRsltRight->expressionResult.strResult) >= 0)
		{
			root->expressionResult.bResult = true;
		}
		break;

	case COMPARE_LESS_THAN:
		if (strcmp(pRsltLeft->expressionResult.strResult, pRsltRight->expressionResult.strResult) < 0)
		{
			root->expressionResult.bResult = true;
		}
		break;

	case COMPARE_LESS_EQUALS:
		if (strcmp(pRsltLeft->expressionResult.strResult, pRsltRight->expressionResult.strResult) <= 0)
		{
			root->expressionResult.bResult = true;
		}
		break;
	}

	if (root->nType == NODE_CALCULATE_OPERATOR)
	{
		root->expressionResult.bResult = true;
		root->expressionResult.nResultType = NODE_LITERAL;
		root->expressionResult.nResultDetailedType = LITERAL;
	}
	else if (root->nType == NODE_COMPARE_OPERATOR)
	{
		root->expressionResult.nResultType = NODE_BOOL;
		root->expressionResult.nResultDetailedType = BOOL_TYPE;
	}
}

void ExpressionTree::EvaluateBoolCondition(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight)
{
	root->expressionResult.bResult = false;

	switch (root->nDetailedType)
	{
	case COMPARE_AND:
		if (pRsltLeft->expressionResult.bResult && pRsltRight->expressionResult.bResult)
		{
			root->expressionResult.bResult = true;
		}
		break;
	case COMPARE_OR:
		if (pRsltLeft->expressionResult.bResult || pRsltRight->expressionResult.bResult)
		{
			root->expressionResult.bResult = true;
		}
		break;
	}

	root->expressionResult.nResultType = NODE_BOOL;
	root->expressionResult.nResultDetailedType = BOOL_TYPE;
}

void ExpressionTree::EvaluateNumericCondition(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight)
{
	// TODO: fix these variables
    int nCase = 0;
    long nLongLeft = 0;
    long nLongRight = 0;
    float nFloatLeft = 0;
    float nFloatRight = 0;

    nCase = nCase & 0xffff;

    if (pRsltLeft->expressionResult.nResultDetailedType == NUMBER_LONG)
    {
        nLongLeft = pRsltLeft->expressionResult.nResultLong;
        nCase = nCase | CASE_L_LONG;
    }
    else if (pRsltLeft->expressionResult.nResultDetailedType == NUMBER_FLOAT)
    {
        nFloatLeft = pRsltLeft->expressionResult.nResultFloat;
        nCase = nCase | CASE_L_FLOAT;
    }

    if (pRsltRight->expressionResult.nResultDetailedType == NUMBER_LONG)
    {
        nLongRight = pRsltRight->expressionResult.nResultLong;
        nCase = nCase | CASE_R_LONG;
    }
    else if (pRsltRight->expressionResult.nResultDetailedType == NUMBER_FLOAT)
    {
        nFloatRight = pRsltRight->expressionResult.nResultFloat;
        nCase = nCase | CASE_R_FLOAT;
    }

    if (nCase == CASE_L_LONG_R_LONG)
    {
        root->expressionResult.nResultType = NODE_NUMBER;
        root->expressionResult.nResultDetailedType = NUMBER_LONG;
    }
    else if (nCase == CASE_L_LONG_R_FLOAT || nCase == CASE_L_FLOAT_R_LONG || nCase == CASE_L_FLOAT_R_FLOAT)
    {
        root->expressionResult.nResultType = NODE_NUMBER;
        root->expressionResult.nResultDetailedType = NUMBER_FLOAT;
    }

    root->expressionResult.bResult = false;

    switch (root->nDetailedType)
    {
    case OPERATOR_PLUS:
        switch (nCase)
        {
        case CASE_L_LONG_R_LONG:
            root->expressionResult.nResultLong = nLongLeft + nLongRight;
            break;
        case CASE_L_LONG_R_FLOAT:
            root->expressionResult.nResultFloat = nLongLeft + nFloatRight;
            break;
        case CASE_L_FLOAT_R_LONG:
            root->expressionResult.nResultFloat = nFloatLeft + nLongRight;
            break;
        case CASE_L_FLOAT_R_FLOAT:
            root->expressionResult.nResultFloat = nFloatLeft + nFloatRight;
            break;
        }
        break;
    case OPERATOR_MINUS:
        switch (nCase)
        {
        case CASE_L_LONG_R_LONG:
            root->expressionResult.nResultLong = nLongLeft - nLongRight;
            break;
        case CASE_L_LONG_R_FLOAT:
            root->expressionResult.nResultFloat = nLongLeft - nFloatRight;
            break;
        case CASE_L_FLOAT_R_LONG:
            root->expressionResult.nResultFloat = nFloatLeft - nLongRight;
            break;
        case CASE_L_FLOAT_R_FLOAT:
            root->expressionResult.nResultFloat = nFloatLeft - nFloatRight;
            break;
        }
        break;
    case OPERATOR_TIMES:
        switch (nCase)
        {
        case CASE_L_LONG_R_LONG:
            root->expressionResult.nResultLong = nLongLeft * nLongRight;
            break;
        case CASE_L_LONG_R_FLOAT:
            root->expressionResult.nResultFloat = nLongLeft * nFloatRight;
            break;
        case CASE_L_FLOAT_R_LONG:
            root->expressionResult.nResultFloat = nFloatLeft * nLongRight;
            break;
        case CASE_L_FLOAT_R_FLOAT:
            root->expressionResult.nResultFloat = nFloatLeft * nFloatRight;
            break;
        }
        break;
    case OPERATOR_DIVIDE:
        switch (nCase)
        {
        case CASE_L_LONG_R_LONG:
            root->expressionResult.nResultLong = nLongLeft / nLongRight;
            break;
        case CASE_L_LONG_R_FLOAT:
            root->expressionResult.nResultFloat = nLongLeft / nFloatRight;
            break;
        case CASE_L_FLOAT_R_LONG:
            root->expressionResult.nResultFloat = nFloatLeft / nLongRight;
            break;
        case CASE_L_FLOAT_R_FLOAT:
            root->expressionResult.nResultFloat = nFloatLeft / nFloatRight;
            break;
        }
        break;
    case COMPARE_EQUALS:
        switch (nCase)
        {
        case CASE_L_LONG_R_LONG:
            root->expressionResult.bResult = nLongLeft == nLongRight;
            break;
        case CASE_L_LONG_R_FLOAT:
            root->expressionResult.bResult = nLongLeft == nFloatRight;
            break;
        case CASE_L_FLOAT_R_LONG:
            root->expressionResult.bResult = nFloatLeft == nLongRight;
            break;
        case CASE_L_FLOAT_R_FLOAT:
            root->expressionResult.bResult = nFloatLeft == nFloatRight;
            break;
        }
        break;
    case COMPARE_NOT_EQUALS:
		switch (nCase)
		{
		case CASE_L_LONG_R_LONG:
			root->expressionResult.bResult = nLongLeft != nLongRight;
			break;
		case CASE_L_LONG_R_FLOAT:
			root->expressionResult.bResult = nLongLeft != nFloatRight;
			break;
		case CASE_L_FLOAT_R_LONG:
			root->expressionResult.bResult = nFloatLeft != nLongRight;
			break;
		case CASE_L_FLOAT_R_FLOAT:
			root->expressionResult.bResult = nFloatLeft != nFloatRight;
			break;
		}
		break;
    case COMPARE_GREATER_THAN:
		switch (nCase)
		{
		case CASE_L_LONG_R_LONG:
			root->expressionResult.bResult = nLongLeft > nLongRight;
			break;
		case CASE_L_LONG_R_FLOAT:
			root->expressionResult.bResult = nLongLeft > nFloatRight;
			break;
		case CASE_L_FLOAT_R_LONG:
			root->expressionResult.bResult = nFloatLeft > nLongRight;
			break;
		case CASE_L_FLOAT_R_FLOAT:
			root->expressionResult.bResult = nFloatLeft > nFloatRight;
			break;
		}
		break;
    case COMPARE_GREATER_EQUALS:
		switch (nCase)
		{
		case CASE_L_LONG_R_LONG:
			root->expressionResult.bResult = nLongLeft >= nLongRight;
			break;
		case CASE_L_LONG_R_FLOAT:
			root->expressionResult.bResult = nLongLeft >= nFloatRight;
			break;
		case CASE_L_FLOAT_R_LONG:
			root->expressionResult.bResult = nFloatLeft >= nLongRight;
			break;
		case CASE_L_FLOAT_R_FLOAT:
			root->expressionResult.bResult = nFloatLeft >= nFloatRight;
			break;
		}
		break;
    case COMPARE_LESS_THAN:
		switch (nCase)
		{
		case CASE_L_LONG_R_LONG:
			root->expressionResult.bResult = nLongLeft < nLongRight;
			break;
		case CASE_L_LONG_R_FLOAT:
			root->expressionResult.bResult = nLongLeft < nFloatRight;
			break;
		case CASE_L_FLOAT_R_LONG:
			root->expressionResult.bResult = nFloatLeft < nLongRight;
			break;
		case CASE_L_FLOAT_R_FLOAT:
			root->expressionResult.bResult = nFloatLeft < nFloatRight;
			break;
		}
		break;
    case COMPARE_LESS_EQUALS:
		switch (nCase)
		{
		case CASE_L_LONG_R_LONG:
			root->expressionResult.bResult = nLongLeft <= nLongRight;
			break;
		case CASE_L_LONG_R_FLOAT:
			root->expressionResult.bResult = nLongLeft <= nFloatRight;
			break;
		case CASE_L_FLOAT_R_LONG:
			root->expressionResult.bResult = nFloatLeft <= nLongRight;
			break;
		case CASE_L_FLOAT_R_FLOAT:
			root->expressionResult.bResult = nFloatLeft <= nFloatRight;
			break;
		}
		break;
    }

    if (root->nType == NODE_COMPARE_OPERATOR)
    {
        root->expressionResult.nResultType = NODE_BOOL;
        root->expressionResult.nResultDetailedType = BOOL_TYPE;
    }
    else if (root->nType == NODE_CALCULATE_OPERATOR)
    {
        root->expressionResult.bResult = true;
    }

}

void ExpressionTree::EvaluateLogic(expression_node* root, expression_node* pRsltLeft, expression_node* pRsltRight)
{
	if (root->nType == NODE_USER_FUNCTION)
	{

		if (userFuncs.InvokeUserFunction(root, pRsltLeft, pRsltRight))
		{
			if (root->userFuncInfo.nReturnType == FUNC_RTN_STR)
			{
				root->expressionResult.nResultType = NODE_LITERAL;
				root->expressionResult.nResultDetailedType = LITERAL;

				root->expressionResult.bResult = true;
			}
			else if (root->userFuncInfo.nReturnType == FUNC_RTN_NUM)
			{
				root->expressionResult.nResultType = NODE_NUMBER;
				root->expressionResult.nResultDetailedType = root->userFuncInfo.nDetailedReturnType;

				root->expressionResult.bResult = true;
			}
			else if (root->userFuncInfo.nReturnType == FUNC_RTN_BOOL)
			{
				root->expressionResult.nResultType = NODE_BOOL;
				root->expressionResult.nResultDetailedType = BOOL_TYPE;
			}
		}
		else
		{
			root->expressionResult.bResult = false;
		}

	}
	else
	{
		if (!CheckCompareError(pRsltLeft, pRsltRight))
		{
			root->expressionResult.bResult = false;
			return;
		}

		if (pRsltLeft->expressionResult.nResultType == NODE_LITERAL && pRsltRight->expressionResult.nResultType == NODE_LITERAL)
		{
			//if expression is '12'='1'+'2' , strip off both side's single quotations..
			if (pRsltLeft->strVal[0] == CHAR_DOUBLE_QUOTATION &&
				pRsltLeft->strVal[strlen(pRsltLeft->strVal) - 1] == CHAR_DOUBLE_QUOTATION)
			{
				userFuncs.GetStringDoubleQuotationsBothSidesRemoved(pRsltLeft->strVal);
				userFuncs.GetStringDoubleQuotationsBothSidesRemoved(pRsltLeft->expressionResult.strResult);
			}
			if (pRsltRight->strVal[0] == CHAR_DOUBLE_QUOTATION &&
				pRsltRight->strVal[strlen(pRsltRight->strVal) - 1] == CHAR_DOUBLE_QUOTATION)
			{
				userFuncs.GetStringDoubleQuotationsBothSidesRemoved(pRsltRight->strVal);
				userFuncs.GetStringDoubleQuotationsBothSidesRemoved(pRsltRight->expressionResult.strResult);
			}

			EvaluateStringCondition(root, pRsltLeft, pRsltRight);
		}
		else if (pRsltLeft->expressionResult.nResultType == NODE_NUMBER && pRsltRight->expressionResult.nResultType == NODE_NUMBER)
		{
			EvaluateNumericCondition(root, pRsltLeft, pRsltRight);
		}
		else if (pRsltLeft->expressionResult.nResultType == NODE_BOOL && pRsltRight->expressionResult.nResultType == NODE_BOOL)
		{
			EvaluateBoolCondition(root, pRsltLeft, pRsltRight);
		}
	}
}

void ExpressionTree::EvaluateConditionNonRecursive(expression_node* root)
{
	std::stack<expression_node*> s;

	while (root != NULL || !s.empty())
	{
		if (root != NULL) // left node push
		{
			s.push(root);
			root = root->left;
			continue;
		}

		while (!s.empty() && s.top()->right == root) // right node pop
		{
			root = s.top();
			s.pop();

			if (root->left && root->right)
			{
				EvaluateConditionNonRecursive(root->right->rightSiblingForMore2funcArgs);
				EvaluateConditionNonRecursive(root->rightSiblingForMore2funcArgs);

				EvaluateLogic(root, root->left, root->right);

			}
		}

		root = s.empty() ? NULL : s.top()->right;
	}
}

bool ExpressionTree::EvaluateExpression()
{
	EvaluateConditionNonRecursive(root_node);

	return root_node->expressionResult.bResult;
}

expression_result* ExpressionTree::GetResult()
{
	return &root_node->expressionResult;
}


bool ExpressionTree::BuildExpressionTree()
{
	nDepthOfTree = 0;
	DeleteExpressionTree(root_node);

	std::stack<expression_node*> treeNodeStack;
	std::stack<expression_node*> funcArgsNodeStack;

	for (auto it = vecPostfixResult.begin(); it != vecPostfixResult.end(); ++it)
	{
		if (IsUserFunction(&(*it)))
		{
			int nArgCnt = it->userFuncInfo.ntotalInputArgCnt;

			if (nArgCnt == 0)
			{
				expression_node* node = CreateTreeNode();
				expression_node* nodeEmptyR = CreateTreeNode(); // empty node
				expression_node* nodeEmptyL = CreateTreeNode(); // empty node
				nodeEmptyR->nType = NODE_EMPTY;
				nodeEmptyL->nType = NODE_EMPTY;

				node->right = nodeEmptyR;
				node->left = nodeEmptyL;

				node->nType = it->nType;
				node->nDetailedType = it->nDetailedType;
				node->expressionResult.nResultType = it->nType;
				node->expressionResult.nResultDetailedType = it->nDetailedType;

				memcpy(&node->userFuncInfo, &it->userFuncInfo, sizeof(node->userFuncInfo));
				treeNodeStack.push(node);
			}
			else if (nArgCnt == 1)
			{
				int nArgType = it->userFuncInfo.nFuncArgsTypes[0];
				if (treeNodeStack.empty())
				{
					return false;
				}
				expression_node* nodeStack = treeNodeStack.top();

				if (FUNC_ARG_STR == nArgType)
				{
					userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeStack->strVal);
					userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeStack->expressionResult.strResult);
				}

				treeNodeStack.pop();

				expression_node* nodeEmpty = CreateTreeNode(); //empty node
				nodeEmpty->nType = NODE_EMPTY;
				expression_node* node = CreateTreeNode();
				node->left = nodeStack;
				node->right = nodeEmpty;

				node->nType = it->nType;
				node->nDetailedType = it->nDetailedType;
				node->expressionResult.nResultType = it->nType;
				node->expressionResult.nResultDetailedType = it->nDetailedType;

				memcpy(&node->userFuncInfo, &it->userFuncInfo, sizeof(node->userFuncInfo));
				treeNodeStack.push(node);
			}
			else if (nArgCnt == 2)
			{
				int nArgType = it->userFuncInfo.nFuncArgsTypes[1];  // 1
				if (treeNodeStack.empty())
				{
					return false;
				}
				expression_node* nodeArgR = treeNodeStack.top();

				if (FUNC_ARG_STR == nArgType)
				{
					userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeArgR->strVal);
					userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeArgR->expressionResult.strResult);
				}
				treeNodeStack.pop();

				nArgType = it->userFuncInfo.nFuncArgsTypes[0];
				if (treeNodeStack.empty())
				{
					return false;
				}
				expression_node* nodeArgL = treeNodeStack.top();
				if (FUNC_ARG_STR == nArgType)
				{
					userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeArgL->strVal);
					userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeArgL->expressionResult.strResult);
				}
				treeNodeStack.pop();

				expression_node* node = CreateTreeNode();
				node->right = nodeArgR;
				node->left = nodeArgL;

				node->nType = it->nType;
				node->nDetailedType = it->nDetailedType;
				node->expressionResult.nResultType = it->nType;
				node->expressionResult.nResultDetailedType = it->nDetailedType;

				memcpy(&node->userFuncInfo, &it->userFuncInfo, sizeof(node->userFuncInfo));
				treeNodeStack.push(node);
			}
			else if (nArgCnt > 2)
			{
				expression_node* node = CreateTreeNode();
				expression_node* nodeRight = NULL;

				for (int i = 0; i < nArgCnt; i++)
				{
					int nArgType = it->userFuncInfo.nFuncArgsTypes[nArgCnt - i - 1]; //args type XXX

					if (i == nArgCnt - 1) // last pop -> left
					{
						if (treeNodeStack.empty())
						{
							return false;
						}
						expression_node* nodeLeft = treeNodeStack.top();
						treeNodeStack.pop();
						if (FUNC_ARG_STR == nArgType)
						{
							userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeLeft->strVal);
							userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeLeft->expressionResult.strResult);
						}
						node->left = nodeLeft;
					}
					else if (i == nArgCnt - 2) // right
					{
						if (treeNodeStack.empty())
						{
							return false;
						}
						nodeRight = treeNodeStack.top();
						treeNodeStack.pop();
						if (FUNC_ARG_STR == nArgType)
						{
							userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeRight->strVal);
							userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeRight->expressionResult.strResult);
						}
					}
					else
					{
						if (treeNodeStack.empty())
						{
							return false;
						}
						expression_node* nodeForMore2Args = treeNodeStack.top();
						treeNodeStack.pop();
						if (FUNC_ARG_STR == nArgType)
						{
							userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeForMore2Args->strVal);
							userFuncs.GetStringDoubleQuotationsBothSidesRemoved(nodeForMore2Args->expressionResult.strResult);
						}
						funcArgsNodeStack.push(nodeForMore2Args);
					}
				}

				expression_node* nodePosForFuncArgs = NULL;
				while (!funcArgsNodeStack.empty())
				{
					if (nodePosForFuncArgs == NULL)
					{
						nodePosForFuncArgs = nodeRight->rightSiblingForMore2funcArgs = funcArgsNodeStack.top();
					}
					else
					{
						nodePosForFuncArgs->rightSiblingForMore2funcArgs = CreateTreeNode();
						nodePosForFuncArgs->rightSiblingForMore2funcArgs = funcArgsNodeStack.top();
						nodePosForFuncArgs = nodePosForFuncArgs->rightSiblingForMore2funcArgs;
					}

					funcArgsNodeStack.pop();
				}

				node->right = nodeRight;

				node->nType = it->nType;
				node->nDetailedType = it->nDetailedType;
				node->expressionResult.nResultType = it->nType;
				node->expressionResult.nResultDetailedType = it->nDetailedType;

				memcpy(&node->userFuncInfo, &it->userFuncInfo, sizeof(node->userFuncInfo));
				treeNodeStack.push(node);

			}
		}
		else if (IsOperand(&(*it)))
		{
			expression_node* node = CreateTreeNode();

			if (NODE_NUMBER == it->nType)
			{
				if (NUMBER_LONG == it->nDetailedType)
				{
					node->nLongValue = atol(it->strValue);
					node->expressionResult.nResultLong = node->nLongValue;
				}
				else if (NUMBER_FLOAT == it->nDetailedType)
				{
					node->nFloatValue = (float)atof(it->strValue);
					node->expressionResult.nResultFloat = node->nFloatValue;
				}
			}
			else
			{
				memcpy(&node->strVal, it->strValue, sizeof(node->strVal));
				memcpy(&node->expressionResult.strResult, it->strValue, sizeof(node->expressionResult.strResult));
			}

			node->nType = it->nType;
			node->nDetailedType = it->nDetailedType;
			node->expressionResult.nResultType = it->nType;
			node->expressionResult.nResultDetailedType = it->nDetailedType;

			treeNodeStack.push(node);
		}
		else if (IsOperator(&(*it)))
		{
			if (treeNodeStack.empty())
			{
				return false;
			}

			expression_node* node1 = treeNodeStack.top();
			treeNodeStack.pop();

			if (treeNodeStack.empty())
			{
				return false;
			}

			expression_node* node2 = treeNodeStack.top();
			treeNodeStack.pop();

			expression_node* node = CreateTreeNode();
			node->right = node1;
			node->left = node2;

			node->nType = it->nType;
			node->nDetailedType = it->nDetailedType;
			node->expressionResult.nResultType = it->nType;
			node->expressionResult.nResultDetailedType = it->nDetailedType;

			treeNodeStack.push(node);
		}
	}

	if (treeNodeStack.empty())
	{
		return false;
	}
	root_node = treeNodeStack.top();
	treeNodeStack.pop();

	return true;
}


void ExpressionTree::DeleteExpressionTree(expression_node* root)
{
	while (!treeNodeMemRepository.empty())
	{
		expression_node* pDelete = treeNodeMemRepository.top();
		treeNodeMemRepository.pop();

		delete pDelete;

		--memStatus;
	}

}