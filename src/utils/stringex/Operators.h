#pragma once

#include "ExpressionError.h"
#include "ExpressionToken.h"
#include "TokenStream.h"

#include <array>
#include <charconv>
#include <functional>
#include <regex>
#include <string>
#include <vector>

namespace EGL3::Utils::StringEx::Operators {
    struct SubExpressionStart {
        static constexpr std::array<const char*, 1> Monikers = { "(" };
    };

    struct SubExpressionEnd {
        static constexpr std::array<const char*, 1> Monikers = { ")" };
    };

    struct Equal : public std::equal_to<int64_t>, public std::equal_to<std::string> {
        static constexpr std::array<const char*, 3> Monikers = { "==", "=", ":" };
    };

    struct NotEqual : public std::not_equal_to<int64_t>, public std::not_equal_to<std::string> {
        static constexpr std::array<const char*, 2> Monikers = { "!=", "!:" };
    };

    struct Less : public std::less<int64_t> {
        static constexpr std::array<const char*, 1> Monikers = { "<" };
    };

    struct LessOrEqual : public std::less_equal<int64_t> {
        static constexpr std::array<const char*, 2> Monikers = { "<=", "<:" };
    };

    struct Greater : public std::greater<int64_t> {
        static constexpr std::array<const char*, 1> Monikers = { ">" };
    };

    struct GreaterOrEqual : public std::greater_equal<int64_t> {
        static constexpr std::array<const char*, 2> Monikers = { ">=", ">:" };
    };

    struct Or : public std::logical_or<bool> {
        static constexpr std::array<const char*, 3> Monikers = { "OR", "||", "|" };
    };

    struct And : public std::logical_and<bool> {
        static constexpr std::array<const char*, 3> Monikers = { "AND", "&&", "&" };
    };

    struct Not : public std::logical_not<bool> {
        static constexpr std::array<const char*, 2> Monikers = { "NOT", "!" };
    };

    struct Plus : public std::plus<int64_t> {
        static constexpr std::array<const char*, 1> Monikers = { "+" };
    };

    struct Minus : public std::minus<int64_t> {
        static constexpr std::array<const char*, 1> Monikers = { "-" };
    };

    struct Multiply : public std::multiplies<int64_t> {
        static constexpr std::array<const char*, 1> Monikers = { "*" };
    };

    struct Divide {
        static constexpr std::array<const char*, 1> Monikers = { "/" };

        ExpressionError operator()(int64_t A, int64_t B, const ExpressionContext* Ctx, std::any& Out) const {
            if (B) {
                Out = int64_t(A / B);
                return ExpressionError();
            }
            return ExpressionError("Divide by zero");
        }
    };

    struct Modulo {
        static constexpr std::array<const char*, 1> Monikers = { "%" };

        ExpressionError operator()(int64_t A, int64_t B, const ExpressionContext* Ctx, std::any& Out) const {
            if (B) {
                Out = int64_t(A % B);
                return ExpressionError();
            }
            return ExpressionError("Modulo by zero");
        }
    };

    struct FuncRegex {
        static constexpr const char* FunctionName = "Regex";

        ExpressionError operator()(const std::string& V, const ExpressionContext* Ctx, std::any& Out) const {
            std::regex Regex;
            try {
                Regex = std::regex(V);
            }
            catch (const std::regex_error&) {
                return ExpressionError("Invalid regex expression");
            }

            std::smatch Matches;
            if (!std::regex_search(Ctx->Input, Matches, std::regex(V))) {
                Out = false;
                return ExpressionError();
            }
            Ctx->CaptureGroups.clear();
            Ctx->CaptureGroups.reserve(Matches.size());
            for (auto& Match : Matches) {
                Ctx->CaptureGroups.emplace_back(Match);
            }
            Out = true;
            return ExpressionError();
        }
    };

    struct FuncRegexGroupInt64 {
        static constexpr const char* FunctionName = "RegexGroupInt64";

        ExpressionError operator()(const std::string& V, const ExpressionContext* Ctx, std::any& Out) const {
            int GroupIdx = 0;
            auto Result = std::from_chars(V.data(), V.data() + V.size(), GroupIdx);
            if (Result.ptr != V.data() + V.size()) {
                return ExpressionError("Input isn't a number");
            }
            if (Ctx->CaptureGroups.size() <= GroupIdx)
            {
                return ExpressionError("Group idx out of range");
            }
            auto& Group = Ctx->CaptureGroups[GroupIdx];
            int64_t Num;
            Result = std::from_chars(Group.data(), Group.data() + Group.size(), Num);
            if (Result.ptr != Group.data() + Group.size()) {
                return ExpressionError("Couldn't convert group to number");
            }
            Out = Num;
            return ExpressionError();
        }
    };

    struct FuncRegexGroupString {
        static constexpr const char* FunctionName = "RegexGroupString";

        ExpressionError operator()(const std::string& V, const ExpressionContext* Ctx, std::any& Out) const {
            int GroupIdx = 0;
            auto Result = std::from_chars(V.data(), V.data() + V.size(), GroupIdx);
            if (Result.ptr != V.data() + V.size()) {
                return ExpressionError("Input isn't a number");
            }
            if (Ctx->CaptureGroups.size() <= GroupIdx)
            {
                return ExpressionError("Group idx out of range");
            }
            Out = Ctx->CaptureGroups[GroupIdx];
            return ExpressionError();
        }
    };

    struct FuncCustomData {
        std::string Name;
        std::string Parameter;
    };

    struct FuncCustom {
        ExpressionError operator()(const FuncCustomData& V, const ExpressionContext* Ctx, std::any& Out) const {
            auto Itr = Ctx->Functions.find(V.Name);
            if (Itr == Ctx->Functions.end()) {
                return ExpressionError("Custom function doesn't exist");
            }
            Out = Itr->second(V.Parameter);
            return ExpressionError();
        }
    };

    template<typename TSymbol>
    static ExpressionError ConsumeOperator(TokenStream& Stream, std::vector<ExpressionToken>& Tokens)
    {
        for (const char* Moniker : TSymbol::Monikers)
        {
            StringToken OperatorToken = Stream.ParseToken(Moniker);
            if (OperatorToken)
            {
                Stream.Seek(Tokens.emplace_back(OperatorToken, TSymbol()).Token);
            }
        }
        return ExpressionError();
    }

    static ExpressionError ConsumeBool(TokenStream& Stream, std::vector<ExpressionToken>& Tokens)
    {
        static const char* True = "true";
        static const char* False = "false";
        StringToken OperatorToken = Stream.ParseToken(True);
        if (OperatorToken)
        {
            Stream.Seek(Tokens.emplace_back(OperatorToken, true).Token);
        }
        else
        {
            OperatorToken = Stream.ParseToken(False);
            if (OperatorToken)
            {
                Stream.Seek(Tokens.emplace_back(OperatorToken, false).Token);
            }
        }
        return ExpressionError();
    }

    static StringToken ParseNumber(const TokenStream& Stream) {
        double Value = 0;
        auto Result = std::from_chars(Stream.GetReadPos(), Stream.GetEnd(), Value);
        auto ParsedLen = Result.ptr - Stream.GetReadPos();

        return ParsedLen > 0 ? Stream.GenerateToken(ParsedLen) : StringToken();
    }

    static ExpressionError ConsumeNumber(TokenStream& Stream, std::vector<ExpressionToken>& Tokens)
    {
        StringToken NumberToken = ParseNumber(Stream);
        if (NumberToken)
        {
            int64_t Num;
            auto Result = std::from_chars(NumberToken.GetStart(), NumberToken.GetEnd(), Num);
            if (Result.ptr != NumberToken.GetEnd()) {
                return ExpressionError("Could not consume number");
            }
            Stream.Seek(Tokens.emplace_back(NumberToken, Num).Token);
        }
        return ExpressionError();
    }

    static std::string TrimQuotes(const StringToken& Input) {
        const size_t Size = Input.GetEnd() - Input.GetStart();
        size_t Start = 0;
        size_t Count = Size;
        if (Count > 0) {
            if (Input.GetStart()[0] == '"')
            {
                Start++;
                Count--;
            }

            if (Size > 1 && Input.GetStart()[Size - 1] == '"')
            {
                Count--;
            }
        }
        return std::string(Input.GetStart() + Start, Count);
    }

    static ExpressionError ConsumeString(TokenStream& Stream, std::vector<ExpressionToken>& Tokens)
    {
        static const char DoubleQuote = '"';
        static const char Backslash = '\\';
        if (Stream.PeekChar() == DoubleQuote)
        {
            bool bOpenedQuote = false;
            bool bClosedQuote = false;
            bool bEscaped = false;
            StringToken StringToken = Stream.ParseToken([&](char CurrentChar)
            {
                if (bEscaped)
                {
                    bEscaped = false;
                    return ParseState::Continue;
                }
                else if (CurrentChar == Backslash)
                {
                    bEscaped = true;
                    return ParseState::Continue;
                }
                else if (CurrentChar == DoubleQuote)
                {
                    if (!bOpenedQuote)
                    {
                        bOpenedQuote = true;
                        return ParseState::Continue;
                    }
                    else
                    {
                        bClosedQuote = true;
                        return ParseState::StopAfter;
                    }
                }
                else if (bOpenedQuote)
                {
                    return ParseState::Continue;
                }
                else {
                    return ParseState::Cancel;
                }
            });

            if (StringToken && bClosedQuote)
            {
                Stream.Seek(Tokens.emplace_back(StringToken, TrimQuotes(StringToken)).Token);
            }
        }
        return ExpressionError();
    }

    static size_t FindMatchingClosingParenthesis(const TokenStream& Stream, const size_t StartSearch) {
        // https://github.com/EpicGames/UnrealEngine/blob/2bf1a5b83a7076a0fd275887b373f8ec9e99d431/Engine/Source/Runtime/Core/Private/Containers/String.cpp#L1505
        
        const char* const StartPosition = Stream.GetStart() + StartSearch;
        const char* CurrPosition = StartPosition;
        int ParenthesisCount = 0;

        // Move to first open parenthesis
        while (*CurrPosition != 0 && *CurrPosition != '(')
        {
            ++CurrPosition;
        }

        // Did we find the open parenthesis
        if (*CurrPosition == '(')
        {
            ++ParenthesisCount;
            ++CurrPosition;

            while (*CurrPosition != 0 && ParenthesisCount > 0)
            {
                if (*CurrPosition == '(')
                {
                    ++ParenthesisCount;
                }
                else if (*CurrPosition == ')')
                {
                    --ParenthesisCount;
                }
                ++CurrPosition;
            }

            // Did we find the matching close parenthesis
            if (ParenthesisCount == 0 && *(CurrPosition - 1) == ')')
            {
                return StartSearch + ((CurrPosition - 1) - StartPosition);
            }
        }

        return -1;
    }

    template<typename TSymbol>
    static ExpressionError ConsumeFunction(TokenStream& Stream, std::vector<ExpressionToken>& Tokens)
    {
        static const char OpenParenthesis = '(';
        StringToken OperatorToken = Stream.ParseToken(TSymbol::FunctionName);
        if (OperatorToken && Stream.PeekChar(strlen(TSymbol::FunctionName)) == OpenParenthesis)
        {
            Stream.Seek(Tokens.emplace_back(OperatorToken, TSymbol()).Token);
            const size_t ParameterBegin = Stream.Tell(); // Open parenthesis here
            const size_t ClosingParenthesis = FindMatchingClosingParenthesis(Stream, ParameterBegin);
            if (ClosingParenthesis != -1)
            {
                const size_t ParameterLen = (ClosingParenthesis + 1) - ParamBegin;
                StringToken ParameterToken = Stream.GenerateToken(ParameterLen);
                Stream.Seek(Tokens.emplace_back(ParameterToken, std::string(ParameterToken.GetStart() + 1, ParameterLen - 2)).Token);
            }
            else
            {
                return ExpressionError("Missing close parenthesis for function");
            }
        }
        return ExpressionError();
    }

    static ExpressionError ConsumeCustomFunction(TokenStream& Stream, std::vector<ExpressionToken>& Tokens, const std::unordered_map<std::string, std::function<bool(const std::string&)>>& Functions)
    {
        static const char OpenParenthesis = '(';
        for (auto& Function : Functions) {
            StringToken OperatorToken = Stream.ParseToken(Function.first.c_str());
            if (OperatorToken && Stream.PeekChar(Function.first.size()) == OpenParenthesis)
            {
                Stream.Seek(Tokens.emplace_back(OperatorToken, FuncCustom()).Token);
                const size_t ParameterBegin = Stream.Tell();
                const size_t ClosingParenthesis = FindMatchingClosingParenthesis(Stream, ParameterBegin);
                if (ClosingParenthesis != -1)
                {
                    const size_t ParameterLen = (ClosingParenthesis + 1) - ParameterBegin;
                    StringToken ParameterToken = Stream.GenerateToken(ParameterLen);
                    Stream.Seek(Tokens.emplace_back(ParameterToken, FuncCustomData{ Function.first, std::string(ParameterToken.GetStart() + 1, ParameterLen - 2) }).Token);
                }
                else
                {
                    return ExpressionError("Missing close parenthesis for function");
                }
            }
        }
        return ExpressionError();
    }
}