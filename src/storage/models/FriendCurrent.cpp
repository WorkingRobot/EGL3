#include "FriendCurrent.h"

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

    FriendCurrent::FriendCurrent() :
        FriendReal(GetValidDefaultSummaryData())
    {

    }

    ShowStatus FriendCurrent::GetShowStatus() const {
        return OnlineStatus;
    }

    const std::string& FriendCurrent::GetStatus() const {
        return DisplayStatus;
    }

    const std::string& FriendCurrent::GetKairosAvatar() const {
        return FriendRequested::GetKairosAvatar();
    }

    const std::string& FriendCurrent::GetKairosBackground() const {
        return FriendRequested::GetKairosBackground();
    }

    void FriendCurrent::SetCurrentUserData(const std::string& AccountId, const std::string& Username) {
        this->AccountId = AccountId;
        this->Username = Username;
        
        OnUpdate.emit();
    }

    void FriendCurrent::SetDisplayStatus(const std::string& NewStatus) {
        DisplayStatus = NewStatus;
    }

    void FriendCurrent::SetShowStatus(ShowStatus NewStatus) {
        OnlineStatus = NewStatus;
    }

    Presence FriendCurrent::BuildPresence() const {
        Presence Ret;
        Ret.ShowStatus = GetShowStatus();
        Ret.Status.SetProductName("EGL3");
        Ret.Status.SetStatus(GetStatus());
        return Ret;
    }

    Web::Epic::Responses::GetFriendsSummary::RealFriend FriendCurrent::GetValidDefaultSummaryData() {
        Web::Epic::Responses::GetFriendsSummary::RealFriend Ret;
        Ret.AccountId = "00000000000000000000000000000000";
        Ret.DisplayName = "You";
        return Ret;
    }
}