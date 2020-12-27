#pragma once

#include "FriendBaseUser.h"

namespace EGL3::Storage::Models {
    class FriendRequested : public FriendBaseUser {
    protected:
        // Connections aren't used (yet), so no reason to save them

    public:
        FriendRequested(const Web::Epic::Responses::GetFriendsSummary::RequestedFriend& User);

        FriendRequested(FriendBaseUser&& other);

        std::weak_ordering operator<=>(const FriendRequested& that) const;

    private:
        std::string GetValidUsername(const Web::Epic::Responses::GetFriendsSummary::RequestedFriend& User) const;
    };
}