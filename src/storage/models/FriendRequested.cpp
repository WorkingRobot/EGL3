#include "FriendRequested.h"

namespace EGL3::Storage::Models {
    FriendRequested::FriendRequested(const Web::Epic::Responses::GetFriendsSummary::RequestedFriend& User) :
        FriendBaseUser(User.AccountId, GetValidUsername(User))
    {

    }

    FriendRequested::FriendRequested(FriendBaseUser&& other) : FriendBaseUser(std::move(other)) {

    }

    std::weak_ordering FriendRequested::operator<=>(const FriendRequested& that) const {
        return FriendBaseUser::operator<=>(that);
    }

    std::string FriendRequested::GetValidUsername(const Web::Epic::Responses::GetFriendsSummary::RequestedFriend& User) const {
        if (User.DisplayName.has_value()) {
            return User.DisplayName.value();
        }
        auto ConnItr = std::find_if(User.Connections.begin(), User.Connections.end(), [](const auto& Conn) {
            return Conn.second.Name.has_value();
        });
        if (ConnItr != User.Connections.end()) {
            return ConnItr->second.Name.value();
        }

        return User.AccountId;
    }
}