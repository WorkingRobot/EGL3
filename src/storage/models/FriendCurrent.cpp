#include "FriendCurrent.h"

#include "../../utils/Version.h"

namespace EGL3::Storage::Models {
    FriendCurrent::FriendCurrent() :
        FriendReal(GetValidDefaultSummaryData()),
        Status(Web::Xmpp::Status::Offline)
    {

    }

    Web::Xmpp::Status FriendCurrent::GetStatus() const {
        return Status;
    }

    const std::string& FriendCurrent::GetStatusText() const {
        return StatusText;
    }

    void FriendCurrent::SetCurrentUserData(const std::string& AccountId, const std::string& Username) {
        this->AccountId = AccountId;
        this->Username = Username;
        
        OnUpdate.emit();
    }

    void FriendCurrent::SetStatus(Web::Xmpp::Status NewStatus) {
        Status = NewStatus;
    }

    void FriendCurrent::SetStatusText(const std::string& NewStatusText) {
        StatusText = NewStatusText;
    }

    Web::Epic::Friends::UserPresence FriendCurrent::BuildPresence() const {
        return Web::Epic::Friends::UserPresence{
            .Status = Web::Xmpp::StatusToString(GetStatus()),
            .Activity = GetStatusText(),
            .Properties = {
                { "EOS_Platform", "WIN" },
                { "EOS_ProductVersion", Utils::Version::GetShortAppVersion() },
                { "EOS_ProductName", Utils::Version::GetAppName() },
                { "EOS_Session", "{\"version\":1}" },
                { "EOS_Lobby", "{\"version\":1}" }
            },
            .ConnectionProperties = { }
        };
    }

    Web::Epic::Responses::GetFriendsSummary::RealFriend FriendCurrent::GetValidDefaultSummaryData() {
        Web::Epic::Responses::GetFriendsSummary::RealFriend Ret;
        Ret.AccountId = "00000000000000000000000000000000";
        Ret.DisplayName = "You";
        return Ret;
    }
}