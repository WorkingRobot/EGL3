#pragma once

#include "../../utils/streams/Stream.h"

namespace EGL3::Storage::Models {
    struct AuthUserData {
        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, AuthUserData& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const AuthUserData& Val);

        std::string AccountId;
        std::string DisplayName;
        std::string KairosAvatar;
        std::string KairosBackground;
        std::string RefreshToken;
        std::chrono::utc_clock::time_point RefreshExpireTime;
    };

    class Authorization {
    public:
        Authorization() = default;

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, Authorization& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const Authorization& Val);

        bool IsUserSelected() const;

        const AuthUserData& GetSelectedUser() const;

        AuthUserData& GetSelectedUser();

        const std::vector<AuthUserData>& GetUsers() const;

        std::vector<AuthUserData>& GetUsers();

    private:
        static constexpr uint32_t InvalidUserIdx = -1;

        uint32_t SelectedUserIdx = InvalidUserIdx;
        std::vector<AuthUserData> UserData;
    };
}