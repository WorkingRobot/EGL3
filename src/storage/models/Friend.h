#pragma once

#include "../../utils/Crc32.h"
#include "../../web/epic/responses/Responses.h"
#include "../../web/xmpp/Responses.h"

#include <chrono>
#include <variant>

namespace EGL3::Storage::Models {
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

        // Initial friends endpoint

        std::string AccountId;

        std::string Nickname;

        std::chrono::system_clock::time_point CreationDate;

        RelationDirection Direction;

        bool Favorite;

        RelationStatus Status;

        std::vector<Web::Epic::Responses::GetFriends::Connection> ExternalConnections;

        // Account lookup

        std::string Username;

        // XMPP presence

        std::chrono::system_clock::time_point PresenceUpdated;

        Web::Xmpp::Json::ShowStatus OnlineStatus;

        std::string DisplayStatus;

        std::string ProductId;

        // Kairos information

        Web::Xmpp::Json::KairosProfile Kairos;

        // Callback when this data is updated

        std::optional<std::function<void(const Models::Friend&)>> Updated;

        Friend() :
            AccountId("00000000000000000000000000000000"),
            CreationDate(std::chrono::system_clock::time_point::min()),
            Direction(RelationDirection::INBOUND),
            Favorite(false),
            Status(RelationStatus::ACCEPTED),
            Username("You"),
            PresenceUpdated(std::chrono::system_clock::time_point::min()),
            OnlineStatus(Web::Xmpp::Json::ShowStatus::Offline)
        {}

        Friend(const Web::Epic::Responses::GetFriends::Friend& Friend) :
            AccountId(Friend.AccountId),
            Nickname(Friend.Alias.value_or("")),
            CreationDate(Friend.Created),
            Direction(DirectionToEnum(Friend.Direction)),
            Favorite(Friend.Favorite.value_or(false)),
            Status(StatusToEnum(Friend.Status)),
            ExternalConnections(Friend.Connections.value_or(decltype(ExternalConnections)())), // value or "default"
            Username(Friend.AccountId),
            PresenceUpdated(std::chrono::system_clock::time_point::min()),
            OnlineStatus(Web::Xmpp::Json::ShowStatus::Offline)
        {}

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
                Kairos.Avatar = FriendSetting.Value;
                break;
            case Utils::Crc32("avatarBackground"):
                Kairos.Background = FriendSetting.Value;
                break;
            }

            if (Updated.has_value()) {
                Updated.value()(*this);
            }
        }

        void Update(const Web::Xmpp::Json::StatusData& Status) {
            PresenceUpdated = Status.UpdatedTime;
            OnlineStatus = Status.ShowStatus;
            DisplayStatus = Status.Status;
            ProductId = Status.ProductName.value_or("");
            std::string KairosString;
            if (Status.Properties.GetValue("KairosProfile", KairosString)) {
                rapidjson::Document KairosJson;
                KairosJson.Parse(KairosString.c_str(), KairosString.size());
                if (!KairosJson.HasParseError()) {
                    Web::Xmpp::Json::KairosProfile::Parse(KairosJson, Kairos);
                }
            }

            if (Updated.has_value()) {
                Updated.value()(*this);
            }
        }

        Web::Xmpp::Json::StatusData GetStatusData() const {
            auto Ret = Web::Xmpp::Json::StatusData{
                OnlineStatus,
                PresenceUpdated,
                DisplayStatus,
                false,
                false,
                false,
                "",
                ProductId
            };
            Ret.Properties.SetValue("KairosProfile", Kairos);
            return Ret;
        }

        std::strong_ordering operator<=>(const Friend& that) const {
            if (OnlineStatus != Web::Xmpp::Json::ShowStatus::Offline && that.OnlineStatus != Web::Xmpp::Json::ShowStatus::Offline) {
                if (auto cmp = CompareStrings(that.ProductId, ProductId); cmp != 0)
                    return cmp;
            }
            if (auto cmp = OnlineStatus <=> that.OnlineStatus; cmp != 0)
                return cmp;
            if (auto cmp = Status <=> that.Status; cmp != 0)
                return cmp;
            if (Status == RelationStatus::PENDING) {
                if (auto cmp = Direction <=> that.Direction; cmp != 0)
                    return cmp;
            }
            // Inverse, place favorites first, not last
            if (auto cmp = that.Favorite <=> Favorite; cmp != 0)
                return cmp;
            return CompareStrings(Nickname.empty() ? Username : Nickname, that.Nickname.empty() ? that.Username : that.Nickname);
        }

        // A <=> B
        // NOTE: insensitive
        static std::strong_ordering CompareStrings(const std::string& A, const std::string& B) {
            auto cmp = stricmp(A.c_str(), B.c_str());
            if (cmp < 0) {
                return std::strong_ordering::less;
            }
            else if (cmp > 0) {
                return std::strong_ordering::greater;
            }
            else {
                return std::strong_ordering::equal;
            }
        }

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
    };
}