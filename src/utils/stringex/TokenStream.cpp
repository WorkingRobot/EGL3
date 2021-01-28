#include "TokenStream.h"

#include <cctype>

namespace EGL3::Utils::StringEx {
    TokenStream::TokenStream(std::string_view Data) :
        Start(Data.data()),
        End(Data.data() + Data.size()),
        ReadPos(Data.data())
    {

    }

    StringToken TokenStream::ParseToken(const std::function<ParseState(char)>& Predicate, StringToken* Accumulate) const
    {
        const char* OptReadPos = Accumulate ? Accumulate->GetEnd() : ReadPos;

        if (!IsReadPosValid(OptReadPos))
        {
            return StringToken();
        }

        StringToken Token(OptReadPos);

        while (Token.GetEnd() != End)
        {
            const ParseState State = Predicate(*Token.GetEnd());

            if (State == ParseState::Cancel)
            {
                return StringToken();
            }

            if (State == ParseState::Continue || State == ParseState::StopAfter)
            {
                // Need to include this character in this token
                ++Token.End;
            }

            if (State == ParseState::StopAfter || State == ParseState::StopBefore)
            {
                // Finished parsing the token
                break;
            }
        }

        if (Token.IsValid())
        {
            if (Accumulate)
            {
                Accumulate->Accumulate(Token);
            }
            return Token;
        }
        return StringToken();
    }

    StringToken TokenStream::ParseToken(const char* Symbol, StringToken* Accumulate) const
    {
        const char* OptReadPos = Accumulate ? Accumulate->GetEnd() : ReadPos;

        const size_t Len = strlen(Symbol);
        if (!IsReadPosValid(OptReadPos, Len))
        {
            return StringToken();
        }

        if (*OptReadPos != *Symbol)
        {
            return StringToken();
        }

        StringToken Token(OptReadPos);

        if (strncmp(Token.GetEnd(), Symbol, Len) == 0)
        {
            Token.End += Len;

            if (Accumulate)
            {
                Accumulate->Accumulate(Token);
            }

            return Token;
        }

        return StringToken();
    }

    StringToken TokenStream::ParseTokenIgnoreCase(const char* Symbol, StringToken* Accumulate) const
    {
        const char* OptReadPos = Accumulate ? Accumulate->GetEnd() : ReadPos;

        const size_t Len = strlen(Symbol);
        if (!IsReadPosValid(OptReadPos, Len))
        {
            return StringToken();
        }

        StringToken Token(OptReadPos);

        if (_strnicmp(OptReadPos, Symbol, Len) == 0)
        {
            Token.End += Len;

            if (Accumulate)
            {
                Accumulate->Accumulate(Token);
            }

            return Token;
        }

        return StringToken();
    }

    StringToken TokenStream::ParseWhitespace(StringToken* Accumulate) const
    {
        const char* OptReadPos = Accumulate ? Accumulate->GetEnd() : ReadPos;

        if (IsReadPosValid(OptReadPos))
        {
            return ParseToken([](char InC) { return isspace(InC) ? ParseState::Continue : ParseState::StopBefore; }, Accumulate);
        }

        return StringToken();
    }

    StringToken TokenStream::GenerateToken(size_t NumChars, StringToken* Accumulate) const
    {
        const char* OptReadPos = Accumulate ? Accumulate->GetEnd() : ReadPos;

        if (IsReadPosValid(OptReadPos, NumChars))
        {
            StringToken Token(OptReadPos);
            Token.End += NumChars;
            if (Accumulate)
            {
                Accumulate->Accumulate(Token);
            }
            return Token;
        }

        return StringToken();
    }

    char TokenStream::PeekChar(size_t Offset) const
    {
        if (ReadPos + Offset < End)
        {
            return *(ReadPos + Offset);
        }

        return '\0';
    }

    bool TokenStream::IsReadPosValid(const char* InPos, size_t MinNumChars) const {
        return InPos >= Start && InPos <= End - MinNumChars;
    }

    bool TokenStream::IsEmpty() const
    {
        return ReadPos == End;
    }

    const char* TokenStream::GetReadPos() const
    {
        return ReadPos;
    }

    const char* TokenStream::GetStart() const
    {
        return Start;
    }

    const char* TokenStream::GetEnd() const
    {
        return End;
    }

    size_t TokenStream::Tell() const
    {
        return ReadPos - Start;
    }

    void TokenStream::Seek(const StringToken& Token)
    {
        if (IsReadPosValid(Token.GetEnd(), 0)) {
            ReadPos = Token.GetEnd();
        }
    }
}