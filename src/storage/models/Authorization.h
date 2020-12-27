#pragma once

#include "../../utils/streams/Stream.h"

namespace EGL3::Storage::Models {
    class Authorization {
        std::string AccountId;
        std::string DeviceId;
        std::string Secret;

    public:
        Authorization() = default;

        Authorization(const std::string& AccountId, const std::string& DeviceId, const std::string& Secret);

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, Authorization& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const Authorization& Val);

        const std::string& GetAccountId() const;

        const std::string& GetDeviceId() const;

        const std::string& GetSecret() const;
    };
}