#pragma once

namespace EGL3::Utils::StringEx {
    class ExpressionError {
    public:
        ExpressionError() :
            Errored(false),
            Message(nullptr)
        {

        }

        ExpressionError(const char* Message) :
            Errored(true),
            Message(Message)
        {
            
        }

        bool HasError() const {
            return Errored;
        }

        const char* GetMessage() const {
            return Message;
        }

    private:
        bool Errored;
        const char* Message;
    };
}