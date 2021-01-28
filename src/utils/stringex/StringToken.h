#pragma once

namespace EGL3::Utils::StringEx {
    class StringToken {
    public:
        StringToken() :
            Start(nullptr),
            End(nullptr)
        {

        }

        StringToken(const char* Start, const char* End) :
            Start(Start),
            End(End)
        {

        }

        bool IsValid() const {
            return End != Start;
        }

        const char* GetStart() const {
            return Start;
        }

        const char* GetEnd() const {
            return End;
        }

        void Accumulate(const StringToken& Token) {
            if (Token.End > End) {
                End = Token.End;
            }
        }

        explicit operator bool() const {
            return IsValid();
        }

    private:
        friend class TokenStream;

        StringToken(const char* Start) :
            StringToken(Start, Start)
        {

        }

        const char* Start;
        const char* End;
    };
}