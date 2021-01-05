#include "FriendReal.h"

#include "../../utils/StringCompare.h"

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

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

    const std::string& FriendReal::GetKairosAvatar() const {
        auto PresItr = GetBestEGL3Presence();
        if (PresItr == Presences.end()) {
            return FriendRequested::GetKairosAvatar();
        }

        auto Profile = PresItr->Status.GetKairosProfile();
        if (!Profile) {
            return FriendRequested::GetKairosAvatar();
        }

        return Profile->Avatar;
    }

    const std::string& FriendReal::GetKairosBackground() const {
        auto PresItr = GetBestEGL3Presence();
        if (PresItr == Presences.end()) {
            return FriendRequested::GetKairosBackground();
        }

        auto Profile = PresItr->Status.GetKairosProfile();
        if (!Profile) {
            return FriendRequested::GetKairosBackground();
        }

        return Profile->Background;
    }

    decltype(FriendReal::Presences)::const_iterator FriendReal::GetBestPresence() const {
        return Presences.begin();
    }

    decltype(FriendReal::Presences)::const_iterator FriendReal::GetEGL3Presence() const {
        return std::find_if(Presences.begin(), Presences.end(), [](const Presence& Val) {
            // Currently untrue, clients with a resource that's not "launcher" doesn't get all the info necessary
            return Val.Resource.GetAppId() == "EGL3";
        });
    }

    // Return EGL3's presence, or the best-fit one
    decltype(FriendReal::Presences)::const_iterator FriendReal::GetBestEGL3Presence() const {
        auto PresItr = GetEGL3Presence();
        if (PresItr == Presences.end()) {
            return GetBestPresence();
        }
        return PresItr;
    }

    const std::string_view FriendReal::GetProductId() const {
        auto PresItr = GetBestPresence();
        if (PresItr == Presences.end()) {
            // Allows returning a ref instead of creating a copy
            static const std::string empty = "";
            return empty;
        }
        if (PresItr->Status.GetProductName().empty()) {
            return PresItr->Resource.GetAppId();
        }
        return PresItr->Status.GetProductName();
    }

    const std::string_view FriendReal::GetPlatform() const {
        auto PresItr = GetBestPresence();
        if (PresItr == Presences.end()) {
            // Allows returning a ref instead of creating a copy
            static const std::string empty = "";
            return empty;
        }
        return PresItr->Resource.GetPlatform();
    }

    ShowStatus FriendReal::GetShowStatus() const {
        auto PresItr = GetBestEGL3Presence();

        if (PresItr == Presences.end()) {
            return ShowStatus::Offline;
        }
        return PresItr->ShowStatus;
    }

    const std::string& FriendReal::GetStatus() const {
        auto PresItr = GetBestPresence();

        if (PresItr == Presences.end()) {
            // Allows returning a ref instead of creating a copy
            static const std::string empty = "";
            return empty;
        }
        return PresItr->Status.GetStatus();
    }

    void FriendReal::UpdatePresence(Presence&& Presence) {
        // This erase compares the resource ids, removing the presences coming from the same client
        Presences.erase(Presence);
        if (Presence.ShowStatus != ShowStatus::Offline) {
            Presences.emplace(std::move(Presence));
        }

        UpdateCallback();
    }

    void FriendReal::UpdateInfo(const Web::Epic::Responses::GetFriendsSummary::RealFriend& User) {
        // Nothing else can be updated atm
        Nickname = User.Alias;
    }

    std::weak_ordering FriendReal::operator<=>(const FriendReal& that) const {
        if (GetShowStatus() != ShowStatus::Offline && that.GetShowStatus() != ShowStatus::Offline) {
            if (auto cmp = *GetBestPresence() <=> *that.GetBestPresence(); cmp != 0)
                return cmp;
        }
        if (auto cmp = GetShowStatus() <=> that.GetShowStatus(); cmp != 0)
            return cmp;
        if (auto cmp = Utils::CompareStringsInsensitive(GetDisplayName(), that.GetDisplayName()); cmp != 0)
            return cmp;
        return FriendRequested::operator<=>(that);
    }
}