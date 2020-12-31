#pragma once

#include "ErrorData.h"

namespace EGL3::Web {
    template<class T>
    class Response {
    public:
        bool HasError() const {
            return GetErrorCode() != ErrorData::Status::Success;
        }

        ErrorData::Status GetErrorCode() const {
            return Error.GetStatusCode();
        }

        const ErrorData& GetError() const {
            return Error;
        }

        T& Get() const {
            return *Data;
        }

        T* operator->() const {
            return Data.get();
        }

        Response() :
            Response(ErrorData::Status::Success)
        {

        }

        Response(ErrorData::Status Status) :
            Error(Status),
            Data(nullptr)
        {

        }

        Response(int HttpCode) :
            Response(HttpCode, "")
        {

        }

        Response(int HttpCode, const std::string& ErrorCode) :
            Error(HttpCode, ErrorCode),
            Data(nullptr)
        {

        }

        Response(T&& Data) :
            Error(ErrorData::Status::Success),
            Data(std::make_unique<T>(std::move(Data)))
        {

        }

    private:
        ErrorData Error;
        std::unique_ptr<T> Data;
    };

    // For 204 responses, or update requests (no useful data is returned)
    template<>
    class Response<void> {
    public:
        bool HasError() const {
            return GetErrorCode() != ErrorData::Status::Success;
        }

        ErrorData::Status GetErrorCode() const {
            return Error.GetStatusCode();
        }

        const ErrorData& GetError() const {
            return Error;
        }

        Response() :
            Response(ErrorData::Status::Success)
        {

        }

        Response(ErrorData::Status Status) :
            Error(Status)
        {

        }

        Response(int HttpCode) :
            Response(HttpCode, "")
        {

        }

        Response(int HttpCode, const std::string& ErrorCode) :
            Error(HttpCode, ErrorCode)
        {

        }

    private:
        ErrorData Error;
    };
}
