#include "FriendBaseUser.h"

#include "../../utils/StringCompare.h"

namespace EGL3::Storage::Models {
    FriendBaseUser::FriendBaseUser(const Web::Epic::Responses::GetFriendsSummary::BaseUser& User) :
        AccountId(User.AccountId),
        Username(User.DisplayName.value_or(User.AccountId))
    {

    }

    FriendBaseUser::FriendBaseUser(const std::string& AccountId, const std::string& Username) :
        AccountId(AccountId),
        Username(Username)
    {

    }

    const std::string& FriendBaseUser::GetAccountId() const {
        return AccountId;
    }

    const std::string& FriendBaseUser::GetUsername() const {
        return Username;
    }

    void FriendBaseUser::SetUsername(const std::string& NewUsername) {
        Username = NewUsername;
    }

    const std::string& FriendBaseUser::GetNickname() const {
        // Allows returning a ref instead of creating a copy
        static const std::string empty = "";
        return empty;
    }

    std::weak_ordering FriendBaseUser::operator<=>(const FriendBaseUser& that) const {
        return Utils::CompareStringsInsensitive(Username, that.Username);
    }
}