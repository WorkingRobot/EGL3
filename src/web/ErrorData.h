#pragma once

namespace EGL3::Web {
    class ErrorData {
    public:
        enum class Status : uint8_t {
            Success,
            Failure,
            Cancelled,
            InvalidToken,
            NotJson,
            BadJson
        };

        ErrorData() :
            ErrorData(Status::Success)
        {

        }

        ErrorData(Status StatusCode) :
            StatusCode(StatusCode)
        {

        }

        ErrorData(int HttpCode) :
            ErrorData(HttpCode, "")
        {

        }

        ErrorData(int HttpCode, const std::string& ErrorCode) :
            ErrorCode(ErrorCode),
            StatusCode(Status::Failure),
            HttpCode(HttpCode)
        {

        }

        Status GetStatusCode() const {
            return StatusCode;
        }

        int GetHttpCode() const {
            return StatusCode == Status::Failure ? HttpCode : 0;
        }

        const std::string& GetErrorCode() const {
            static const std::string empty = "";
            return StatusCode == Status::Failure ? ErrorCode : empty;
        }

    private:
        std::string ErrorCode;
        int HttpCode;
        Status StatusCode;
    };
}
