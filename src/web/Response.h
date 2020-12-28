#pragma once

#include "ErrorCode.h"

namespace EGL3::Web {
    template<class T>
    class Response {
    public:
        bool HasError() const {
            return Error != ErrorCode::Success;
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

        Response() :
            Response(ErrorCode::Success)
        {
        }
        Response(ErrorCode Error) :
            Error(Error),
            Data(nullptr)
        {

        }

        Response(T&& Data) :
            Error(ErrorCode::Success),
            Data(std::make_unique<T>(std::move(Data)))
        {

        }

    private:
        ErrorCode Error;
        std::unique_ptr<T> Data;
    };

    // For 204 responses, or update requests (no useful data is returned)
    template<>
    class Response<void> {
    public:
        bool HasError() const {
            return Error != ErrorCode::Success;
        }

        ErrorCode GetErrorCode() const {
            return Error;
        }

        Response() :
            Response(ErrorCode::Success)
        {

        }

        Response(ErrorCode Error) :
            Error(Error)
        {

        }

    private:
        ErrorCode Error;
    };
}
