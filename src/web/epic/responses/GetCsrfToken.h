#pragma once

#include <string>

namespace EGL3::Web::Epic::Responses {
    struct GetCsrfToken {
        std::string XsrfToken;
        std::string EpicSessionAp;
    };
}
