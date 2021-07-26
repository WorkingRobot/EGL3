#pragma once

#include "../../utils/Callback.h"

#include <sigc++/sigc++.h>

namespace EGL3::Storage::Models {
    class FriendBase {
    protected:
        FriendBase() = default;

    public:
        FriendBase(FriendBase&&) = default;
        FriendBase& operator=(FriendBase&&) = default;

        virtual ~FriendBase() = default;

        virtual const std::string& GetAccountId() const = 0;

        virtual const std::string& GetUsername() const = 0;

        virtual const std::string& GetNickname() const = 0;

        const std::string& GetDisplayName() const;

        const std::string& GetSecondaryName() const;

        sigc::signal<void()> OnUpdate;
    };
}