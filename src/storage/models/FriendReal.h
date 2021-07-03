#pragma once

#include "../../web/epic/friends/FriendsClient.h"
#include "FriendRequested.h"

#include <set>

namespace EGL3::Storage::Models {
    class FriendReal : public FriendRequested {
    protected:
        // There's other stuff here, like notes, etc.
        // We don't parse those yet

        std::string Nickname;

        std::set<Web::Epic::Friends::Presence::NamespacePresence, std::greater<Web::Epic::Friends::Presence::NamespacePresence>> NamespacePresences;

    public:
        FriendReal(const Web::Epic::Responses::GetFriendsSummary::RealFriend& User);

        FriendReal(FriendBaseUser&& other);

        const std::string& GetNickname() const override;

        bool HasPresence() const;

        const Web::Epic::Friends::Presence::NamespacePresence& GetPresence() const;

        std::string GetProductName() const;

        std::string GetPlatform() const;

        virtual Web::Xmpp::Status GetStatus() const;

        virtual const std::string& GetStatusText() const;

        void UpdatePresence(const Web::Epic::Friends::Presence& NewPresence);

        // Unlike UpdatePresence, this won't request for a callback!
        void UpdateInfo(const Web::Epic::Responses::GetFriendsSummary::RealFriend& User);

        std::weak_ordering operator<=>(const FriendReal& that) const;
    };
}