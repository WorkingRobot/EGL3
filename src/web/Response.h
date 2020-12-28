#pragma once

#include "ErrorCode.h"

namespace EGL3::Web {
    template<class T>
    class Response {
    public:
        bool HasError() const {
            return Error != SUCCESS;
        }

        ErrorCode GetErrorCode() const {
            return Error;
        }

        T& Get() const {
            return *Data;
        }

        T* operator->() const {
            return Data.get();
        }

        Response() : Response(SUCCESS) {}
        Response(ErrorCode Error) : Error(Error), Data(nullptr) {}
        Response(T&& Data) : Error(SUCCESS), Data(std::make_unique<T>(std::forward<T&&>(Data))) {}

    private:
        ErrorCode Error;
        std::unique_ptr<T> Data;
    };

    // For 204 responses, or update requests (no useful data is returned)
    template<>
    class Response<void> {
    public:
        bool HasError() const {
            return Error != SUCCESS;
        }

        ErrorCode GetErrorCode() const {
            return Error;
        }

        Response() : Response(SUCCESS) {}
        Response(ErrorCode Error) : Error(Error) {}

    private:
        ErrorCode Error;
    };
}
