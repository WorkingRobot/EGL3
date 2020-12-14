#pragma once

#include "../../utils/Crc32.h"
#include "../../utils/StringCompare.h"
#include "../../web/epic/responses/Responses.h"
#include "../../web/xmpp/Responses.h"

#include <chrono>
#include <functional>
#include <set>
#include <vector>

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

    struct Friend {
        enum class RelationDirection : uint8_t {
            INBOUND,
            OUTBOUND,
            UNKNOWN
        };

        enum class RelationStatus : uint8_t {
            ACCEPTED,
            PENDING,
            SUGGESTED,
            UNKNOWN
        };

    private:
        // Initial friends endpoint

        std::string AccountId;

        std::string Nickname;

        std::chrono::system_clock::time_point CreationDate;

        bool Favorite;

        RelationStatus FriendStatus;

        RelationDirection FriendRequestDirection;

        std::vector<Web::Epic::Responses::GetFriends::Connection> ExternalConnections;

        // Account lookup

        std::string Username;

        // XMPP presence

        std::set<Presence, std::greater<Presence>> Presences;

        // Kairos data (from api calls, not XMPP)

        std::string KairosAvatar;

        std::string KairosBackground;

        // Callback when this data is updated

        std::optional<std::function<void(const Models::Friend&)>> Updated;

        static RelationDirection DirectionToEnum(const std::string& Direction) {
            switch (Utils::Crc32(Direction))
            {
            case Utils::Crc32("INBOUND"):
                return RelationDirection::INBOUND;
            case Utils::Crc32("OUTBOUND"):
                return RelationDirection::OUTBOUND;
            default:
                return RelationDirection::UNKNOWN;
            }
        }

        static RelationStatus StatusToEnum(const std::string& Status) {
            switch (Utils::Crc32(Status))
            {
            case Utils::Crc32("ACCEPTED"):
                return RelationStatus::ACCEPTED;
            case Utils::Crc32("PENDING"):
                return RelationStatus::PENDING;
            case Utils::Crc32("SUGGESTED"):
                return RelationStatus::SUGGESTED;
            default:
                return RelationStatus::UNKNOWN;
            }
        }

    public:
        Friend() :
            AccountId("00000000000000000000000000000000"),
            CreationDate(std::chrono::system_clock::time_point::min()),
            Favorite(false),
            FriendStatus(RelationStatus::ACCEPTED),
            FriendRequestDirection(RelationDirection::INBOUND),
            Username("You")
        {}

        Friend(const Web::Epic::Responses::GetFriends::Friend& Friend) :
            AccountId(Friend.AccountId),
            Nickname(Friend.Alias.value_or("")),
            CreationDate(Friend.Created),
            Favorite(Friend.Favorite.value_or(false)),
            FriendStatus(StatusToEnum(Friend.Status)),
            FriendRequestDirection(DirectionToEnum(Friend.Direction)),
            ExternalConnections(Friend.Connections.value_or(decltype(ExternalConnections)())), // value or "default"
            Username(Friend.AccountId)
        {}

        Friend(Friend&&) noexcept = default;

        template<typename... Args>
        void SetUpdateCallback(Args&&... Callback) {
            Updated.emplace(std::forward<Args>(Callback)...);
        }

        void Update(const Web::Epic::Responses::GetAccounts::Account& Friend) {
            AccountId = Friend.Id;
            Username = Friend.GetDisplayName();
            // We don't care too much about their external accounts, the external connections given before are already more than enough

            if (Updated.has_value()) {
                Updated.value()(*this);
            }
        }

        void Update(const Web::Epic::Responses::GetSettingsForAccounts::AccountSetting& FriendSetting) {
            switch (Utils::Crc32(FriendSetting.Key))
            {
            case Utils::Crc32("avatar"):
                KairosAvatar = FriendSetting.Value;
                break;
            case Utils::Crc32("avatarBackground"):
                KairosBackground = FriendSetting.Value;
                break;
            default:
                return;
            }

            if (Updated.has_value()) {
                Updated.value()(*this);
            }
        }

        void Update(Presence&& Presence) {
            // This erase compares the resource ids, removing the presences coming from the same client
            Presences.erase(Presence);
            if (Presence.ShowStatus != ShowStatus::Offline) {
                Presences.emplace(std::move(Presence));
            }

            if (Updated.has_value()) {
                Updated.value()(*this);
            }
        }


        decltype(Presences)::const_iterator GetBestPresence() const {
            return Presences.begin();
        }

        decltype(Presences)::const_iterator GetEGL3Presence() const {
            return std::find_if(Presences.begin(), Presences.end(), [](const Presence& Val) {
                return Val.Resource.GetAppId() == "EGL3";
            });
        }

        // Return EGL3's presence, or the best-fit one
        decltype(Presences)::const_iterator GetBestEGL3Presence() const {
            auto PresItr = GetEGL3Presence();
            if (PresItr == Presences.end()) {
                return GetBestPresence();
            }
            return PresItr;
        }

        // Get the best available Kairos profile
        // EGL3 gets highest priority
        const PresenceKairosProfile* GetValidKairosProfile() const {
            auto PresItr = GetEGL3Presence();
            if (PresItr == Presences.end()) {
                return nullptr;
            }
            auto Ret = PresItr->Status.GetKairosProfile();
            if (Ret) {
                return Ret;
            }
            for (auto& Pres : Presences) {
                Ret = Pres.Status.GetKairosProfile();
                if (Ret) {
                    return Ret;
                }
            }
            return nullptr;
        }

        const std::string& GetAccountId() const {
            return AccountId;
        }

        bool IsFavorited() const {
            return Favorite;
        }

        RelationStatus GetRelationStatus() const {
            return FriendStatus;
        }

        RelationDirection GetRelationDirection() const {
            return FriendRequestDirection;
        }

        const std::string_view& GetProductId() const {
            auto PresItr = this->GetBestPresence();
            if (PresItr == Presences.end()) {
                // Allows returning a ref instead of creating a copy
                static const std::string empty = "";
                return empty;
            }
            return PresItr->Resource.GetAppId();
        }

        const std::string_view& GetPlatform() const {
            auto PresItr = this->GetBestPresence();
            if (PresItr == Presences.end()) {
                // Allows returning a ref instead of creating a copy
                static const std::string empty = "";
                return empty;
            }
            return PresItr->Resource.GetPlatform();
        }

        virtual ShowStatus GetShowStatus() const {
            auto PresItr = GetBestEGL3Presence();

            if (PresItr == Presences.end()) {
                return ShowStatus::Offline;
            }
            return PresItr->ShowStatus;
        }

        virtual const std::string& GetStatus() const {
            auto PresItr = GetBestPresence();
            if (PresItr == Presences.end()) {
                // Allows returning a ref instead of creating a copy
                static const std::string empty = "";
                return empty;
            }
            return PresItr->Status.GetStatus();
        }

        virtual const std::string& GetKairosAvatar() const {
            auto Profile = GetValidKairosProfile();
            if (Profile) {
                return Profile->Avatar;
            }
            return KairosAvatar;
        }

        virtual const std::string& GetKairosBackground() const {
            auto Profile = GetValidKairosProfile();
            if (Profile) {
                return Profile->Background;
            }
            return KairosBackground;
        }

        const std::string GetKairosAvatarUrl() const {
            return PresenceKairosProfile::GetKairosAvatarUrl(GetKairosAvatar());
        }

        const std::string GetKairosBackgroundUrl() const {
            return PresenceKairosProfile::GetKairosBackgroundUrl(GetKairosBackground());
        }

        const std::string& GetDisplayName() const {
            return Nickname.empty() ? Username : Nickname;
        }

        const std::string& GetAlternateName() const {
            // Allows returning a ref instead of creating a copy
            static const std::string empty = "";
            return Nickname.empty() ? empty : Username;
        }

        std::weak_ordering operator<=>(const Friend& that) const {
            if (GetShowStatus() != ShowStatus::Offline && that.GetShowStatus() != ShowStatus::Offline) {
                if (auto cmp = *GetBestPresence() <=> *that.GetBestPresence(); cmp != 0)
                    return cmp;
            }
            if (auto cmp = GetShowStatus() <=> that.GetShowStatus(); cmp != 0)
                return cmp;
            if (auto cmp = that.FriendStatus <=> FriendStatus; cmp != 0) // Accepted takes priority over suggested
                return cmp;
            if (FriendStatus == RelationStatus::PENDING) {
                if (auto cmp = that.FriendRequestDirection <=> FriendRequestDirection; cmp != 0)
                    return cmp;
            }
            if (auto cmp = that.Favorite <=> Favorite; cmp != 0)
                return cmp;
            if (auto cmp = Utils::CompareStringsInsensitive(that.GetDisplayName(), GetDisplayName()); cmp != 0)
                return cmp;
            return Utils::CompareStringsInsensitive(that.Username, Username);
        }
    };
}