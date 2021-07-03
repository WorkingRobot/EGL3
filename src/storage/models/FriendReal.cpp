#include "FriendReal.h"

#include "../../utils/StringCompare.h"

namespace EGL3::Storage::Models {
    FriendReal::FriendReal(const Web::Epic::Responses::GetFriendsSummary::RealFriend& User) :
        FriendRequested(User),
        Nickname(User.Alias)
    {
            
    }

    FriendReal::FriendReal(FriendBaseUser&& other) : FriendRequested(std::move(other)) {

    }

    const std::string& FriendReal::GetNickname() const {
        return Nickname;
    }

    bool FriendReal::HasPresence() const
    {
        return !NamespacePresences.empty();
    }

    const Web::Epic::Friends::Presence::NamespacePresence& FriendReal::GetPresence() const {
        return *NamespacePresences.begin();
    }

    std::string FriendReal::GetProductId() const
    {
        return HasPresence() ? GetPresence().GetProductId() : "";
    }

    std::string FriendReal::GetPlatformId() const
    {
        return HasPresence() ? GetPresence().GetPlatformId() : "";
    }

    Web::Xmpp::Status FriendReal::GetStatus() const {
        // Yes, EGL really ignores the base status and instead prioritizes the top namespace presence
        return HasPresence() ? Web::Xmpp::StringToStatus(GetPresence().Status) : Web::Xmpp::Status::Offline;
    }

    const std::string& FriendReal::GetStatusText() const {
        static const std::string Empty = "";
        return HasPresence() ? GetPresence().Activity.Value : Empty;
    }

    void FriendReal::UpdatePresence(const Web::Epic::Friends::Presence& NewPresence) {
        NamespacePresences.clear();
        for (auto& Presence : NewPresence.NamespacePresences) {
            NamespacePresences.emplace(Presence);
        }

        OnUpdate.emit();
    }

    void FriendReal::UpdateInfo(const Web::Epic::Responses::GetFriendsSummary::RealFriend& User) {
        // Nothing else can be updated atm
        Nickname = User.Alias;
    }

    std::weak_ordering FriendReal::operator<=>(const FriendReal& that) const {
        if (GetStatus() != Web::Xmpp::Status::Offline && that.GetStatus() != Web::Xmpp::Status::Offline) {
            if (auto cmp = GetPresence() <=> that.GetPresence(); cmp != 0)
                return cmp;
        }
        if (auto cmp = GetStatus() <=> that.GetStatus(); cmp != 0)
            return cmp;
        if (auto cmp = Utils::CompareStringsInsensitive(GetDisplayName(), that.GetDisplayName()); cmp != 0)
            return cmp;
        return FriendRequested::operator<=>(that);
    }
}