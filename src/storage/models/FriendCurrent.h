#pragma once

#include "FriendReal.h"

namespace EGL3::Storage::Models {
    class FriendCurrent : public FriendReal {
        std::string DisplayStatus;

        Web::Xmpp::Json::ShowStatus OnlineStatus;

    public:
        FriendCurrent();

        Web::Xmpp::Json::ShowStatus GetShowStatus() const override;

        const std::string& GetStatus() const override;

        const std::string& GetKairosAvatar() const override;

        const std::string& GetKairosBackground() const override;

        void SetCurrentUserData(const std::string& AccountId, const std::string& Username);

        void SetDisplayStatus(const std::string& NewStatus);

        void SetShowStatus(Web::Xmpp::Json::ShowStatus NewStatus);

        Web::Xmpp::Json::Presence BuildPresence() const;

    private:
        static Web::Epic::Responses::GetFriendsSummary::RealFriend GetValidDefaultSummaryData();
    };
}