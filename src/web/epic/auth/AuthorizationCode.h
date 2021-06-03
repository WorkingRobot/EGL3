#pragma once

#include "../../Http.h"

#include <atomic>
#include <string>

namespace EGL3::Web::Epic::Auth {
    class AuthorizationCode {
    public:
        enum ErrorCode {
            SUCCESS,
            CANCELLED,
            AUTH_CODE_NOT_200,
            AUTH_CODE_JSON
        };

        AuthorizationCode(const cpr::Authentication& AuthClient, const std::string& Code);

        ~AuthorizationCode();

        const std::shared_future<ErrorCode>& GetOAuthResponseFuture() const;

        const rapidjson::Document& GetOAuthResponse() const;

    private:
        ErrorCode RunOAuthResponseTask();

        std::atomic_bool Cancelled;

        std::shared_future<ErrorCode> OAuthResponseFuture;
        rapidjson::Document OAuthResponse;

        cpr::Authentication AuthClient;
        std::string Code;
    };
}
