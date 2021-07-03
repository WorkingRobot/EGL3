#pragma once

#include "FriendReal.h"

namespace EGL3::Storage::Models {
    class FriendCurrent : public FriendReal {
        Web::Xmpp::Status Status;
        std::string StatusText;

    public:
        FriendCurrent();

        Web::Xmpp::Status GetStatus() const override;

        const std::string& GetStatusText() const override;

        void SetCurrentUserData(const std::string& AccountId, const std::string& Username);

        void SetStatus(Web::Xmpp::Status NewStatus);

        void SetStatusText(const std::string& NewStatusText);

        Web::Epic::Friends::UserPresence BuildPresence() const;

    private:
        static Web::Epic::Responses::GetFriendsSummary::RealFriend GetValidDefaultSummaryData();
    };
}