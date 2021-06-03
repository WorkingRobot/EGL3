#pragma once

#include "../../Http.h"

#include <atomic>
#include <string>

namespace EGL3::Web::Epic::Auth {
    class RefreshToken {
    public:
        enum ErrorCode {
            SUCCESS,
            CANCELLED,
            REFRESH_CODE_NOT_200,
            REFRESH_CODE_JSON
        };

        RefreshToken(const cpr::Authentication& AuthClient, const std::string& Code);

        ~RefreshToken();

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
