#pragma once

#include "StringToken.h"

#include <functional>

namespace EGL3::Utils::StringEx {
    enum class ParseState : uint8_t {
        Continue,
        StopAfter,
        StopBefore,
        Cancel,
    };

    class TokenStream {
    public:
        TokenStream(std::string_view Data);

        StringToken ParseToken(const std::function<ParseState(char)>& Predicate, StringToken* Accumulate = nullptr) const;

        StringToken ParseToken(const char* Symbol, StringToken* Accumulate = nullptr) const;
        StringToken ParseTokenIgnoreCase(const char* Symbol, StringToken* Accumulate = nullptr) const;

        StringToken ParseWhitespace(StringToken* Accumulate = nullptr) const;

        StringToken GenerateToken(size_t NumChars, StringToken* Accumulate = nullptr) const;

        char PeekChar(size_t Offset = 0) const;

        bool IsReadPosValid(const char* InPos, size_t MinNumChars = 1) const;

        bool IsEmpty() const;

        const char* GetReadPos() const;

        const char* GetStart() const;

        const char* GetEnd() const;

        size_t Tell() const;

        void Seek(const StringToken& Token);

    private:
        const char* Start;
        const char* End;
        const char* ReadPos;
    };
}