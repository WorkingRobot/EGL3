#pragma once

#include "../../web/xmpp/Presence.h"
#include "../../web/xmpp/ShowStatus.h"
#include "FriendRequested.h"

#include <set>

namespace EGL3::Storage::Models {
    class FriendReal : public FriendRequested {
    protected:
        // There's other stuff here, like notes, etc.
        // We don't parse those yet

        std::string Nickname;

        std::set<Web::Xmpp::Json::Presence, std::greater<Web::Xmpp::Json::Presence>> Presences;

    public:
        FriendReal(const Web::Epic::Responses::GetFriendsSummary::RealFriend& User);

        const std::string& GetNickname() const override;

        const std::string& GetKairosAvatar() const override;

        const std::string& GetKairosBackground() const override;

        decltype(Presences)::const_iterator GetBestPresence() const;

        decltype(Presences)::const_iterator GetEGL3Presence() const;

        // Return EGL3's presence, or the best-fit one
        decltype(Presences)::const_iterator GetBestEGL3Presence() const;

        const std::string_view GetProductId() const;

        const std::string_view GetPlatform() const;

        virtual Web::Xmpp::Json::ShowStatus GetShowStatus() const;

        virtual const std::string& GetStatus() const;

        void UpdatePresence(Web::Xmpp::Json::Presence&& Presence);

        std::weak_ordering operator<=>(const FriendReal& that) const;
    };
}