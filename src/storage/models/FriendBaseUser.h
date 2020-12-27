#pragma once

#include "../../web/epic/responses/GetFriendsSummary.h"
#include "FriendBase.h"

namespace EGL3::Storage::Models {
    class FriendBaseUser : public FriendBase {
    protected:
        std::string AccountId;

        std::string Username;

    public:
        FriendBaseUser(const Web::Epic::Responses::GetFriendsSummary::BaseUser& User);

        FriendBaseUser(const std::string& AccountId, const std::string& Username);

        const std::string& GetAccountId() const override;

        const std::string& GetUsername() const override;

        void SetUsername(const std::string& NewUsername);

        const std::string& GetNickname() const override;

        std::weak_ordering operator<=>(const FriendBaseUser& that) const;
    };
}