#pragma once

#include "../../utils/Crc32.h"
#include "../../utils/StringCompare.h"
#include "../../web/epic/responses/Responses.h"
#include "../../web/xmpp/Responses.h"

#include <functional>
#include <set>
#include <variant>
#include <vector>

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

    class BaseFriend {
    protected:
        using CallbackFunc = std::function<void(const BaseFriend&)>;

        std::optional<CallbackFunc> OnUpdate;

        std::string KairosAvatar, KairosBackground;

        void UpdateCallback() const {
            if (OnUpdate.has_value()) {
                OnUpdate.value()(*this);
            }
        }

    public:
        virtual const std::string& GetAccountId() const = 0;

        virtual const std::string& GetUsername() const = 0;

        virtual const std::string& GetNickname() const = 0;

        virtual const std::string& GetKairosAvatar() const {
            return KairosAvatar;
        }

        virtual const std::string& GetKairosBackground() const {
            return KairosBackground;
        }

        void SetKairosAvatar(const std::string& Avatar) {
            KairosAvatar = Avatar;
        }

        void SetKairosBackground(const std::string& Background) {
            KairosBackground = Background;
        }

        const std::string GetKairosAvatarUrl() const {
            return PresenceKairosProfile::GetKairosAvatarUrl(GetKairosAvatar());
        }

        const std::string GetKairosBackgroundUrl() const {
            return PresenceKairosProfile::GetKairosBackgroundUrl(GetKairosBackground());
        }

        const std::string& GetDisplayName() const {
            if (GetNickname().empty()) {
                return GetUsername();
            }
            return GetNickname();
        }

        const std::string& GetSecondaryName() const {
            if (GetNickname().empty()) {
                return GetNickname(); // Returns ""
            }
            return GetUsername();
        }

        template<typename... Args>
        void SetUpdateCallback(Args&&... Callback) {
            OnUpdate.emplace(std::forward<Args>(Callback)...);
        }

        void UpdateAccountSetting(const Web::Epic::Responses::GetSettingsForAccounts::AccountSetting& FriendSetting) {
            switch (Utils::Crc32(FriendSetting.Key))
            {
            case Utils::Crc32("avatar"):
                SetKairosAvatar(FriendSetting.Value);
                break;
            case Utils::Crc32("avatarBackground"):
                SetKairosBackground(FriendSetting.Value);
                break;
            default:
                return;
            }

            UpdateCallback();
        }
    };

    class FriendBaseUser : public BaseFriend {
    protected:
        std::string AccountId;

        std::string Username;

        FriendBaseUser(const std::string& AccountId, const std::string& Username) :
            AccountId(AccountId),
            Username(Username)
        {

        }

    public:
        FriendBaseUser(const Web::Epic::Responses::GetFriendsSummary::BaseUser& User) :
            AccountId(User.AccountId),
            Username(User.DisplayName.value_or(User.AccountId))
        {

        }

        const std::string& GetAccountId() const override {
            return AccountId;
        }

        const std::string& GetUsername() const override {
            return Username;
        }

        const std::string& GetNickname() const override {
            // Allows returning a ref instead of creating a copy
            static const std::string empty = "";
            return empty;
        }

        std::weak_ordering operator<=>(const FriendBaseUser& that) const {
            return Utils::CompareStringsInsensitive(Username, that.Username);
        }
    };

    class FriendSuggested : public FriendBaseUser {
    protected:
        // Connections aren't used (yet), so no reason to save them

    public:
        FriendSuggested(const Web::Epic::Responses::GetFriendsSummary::SuggestedFriend& User) :
            FriendBaseUser(User.AccountId, GetValidUsername(User))
        {

        }

        std::weak_ordering operator<=>(const FriendSuggested& that) const {
            return FriendBaseUser::operator<=>(that);
        }

    private:
        std::string GetValidUsername(const Web::Epic::Responses::GetFriendsSummary::SuggestedFriend& User) const {
            if (User.DisplayName.has_value()) {
                return User.DisplayName.value();
            }
            auto ConnItr = std::find_if(User.Connections.begin(), User.Connections.end(), [](const auto& Conn) {
                return Conn.second.Name.has_value();
            });
            if (ConnItr != User.Connections.end()) {
                return ConnItr->second.Name.value();
            }
            else {
                return User.AccountId;
            }
        }
    };

    // No extra fields, no reason to define another class
    using FriendRequested = FriendSuggested;

    class FriendReal : public FriendRequested {
    protected:
        // There's other stuff here, like notes, etc.
        // We don't parse those yet

        std::string Nickname;

        std::set<Presence, std::greater<Presence>> Presences;

    public:
        FriendReal(const Web::Epic::Responses::GetFriendsSummary::RealFriend& User) :
            FriendRequested(User),
            Nickname(User.Alias)
        {
            
        }

        const std::string& GetNickname() const override {
            return Nickname;
        }

        decltype(Presences)::const_iterator GetBestPresence() const {
            return Presences.begin();
        }

        decltype(Presences)::const_iterator GetEGL3Presence() const {
            return std::find_if(Presences.begin(), Presences.end(), [](const Presence& Val) {
                // Currently untrue, clients with a resource that's not "launcher" doesn't get all the info necessary
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

        const std::string_view& GetProductId() const {
            auto PresItr = GetBestPresence();
            if (PresItr == Presences.end()) {
                // Allows returning a ref instead of creating a copy
                static const std::string empty = "";
                return empty;
            }
            return PresItr->Resource.GetAppId();
        }

        const std::string_view& GetPlatform() const {
            auto PresItr = GetBestPresence();
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

        void UpdatePresence(Presence&& Presence) {
            // This erase compares the resource ids, removing the presences coming from the same client
            Presences.erase(Presence);
            if (Presence.ShowStatus != ShowStatus::Offline) {
                Presences.emplace(std::move(Presence));
            }

            UpdateCallback();
        }

        std::weak_ordering operator<=>(const FriendReal& that) const {
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
    };

    class FriendCurrent : public FriendReal {
        std::string DisplayStatus;

        ShowStatus OnlineStatus;

    public:
        FriendCurrent() :
            FriendReal(GetValidDefaultSummaryData())
        {

        }

        ShowStatus GetShowStatus() const override {
            return OnlineStatus;
        }

        const std::string& GetStatus() const override {
            return DisplayStatus;
        }

        void SetCurrentUserData(const std::string& AccountId, const std::string& Username) {
            this->AccountId = AccountId;
            this->Username = Username;

            UpdateCallback();
        }

        void SetDisplayStatus(const std::string& NewStatus) {
            DisplayStatus = NewStatus;
        }

        void SetShowStatus(ShowStatus NewStatus) {
            OnlineStatus = NewStatus;
        }

        Presence BuildPresence() const {
            Presence Ret;
            Ret.ShowStatus = GetShowStatus();
            Ret.Status.SetStatus(GetStatus());
            PresenceKairosProfile NewKairosProfile(GetKairosAvatar(), GetKairosBackground());
            Ret.Status.SetKairosProfile(NewKairosProfile);
            return Ret;
        }

    private:
        static Web::Epic::Responses::GetFriendsSummary::RealFriend GetValidDefaultSummaryData() {
            Web::Epic::Responses::GetFriendsSummary::RealFriend Ret;
            Ret.AccountId = "00000000000000000000000000000000";
            Ret.DisplayName = "You";
            Ret.MutualFriendCount = 0;
            return Ret;
        }
    };

    enum class FriendType : uint8_t {
        BLOCKED,
        SUGGESTED,
        OUTBOUND,
        INBOUND,
        NORMAL,
        CURRENT
    };

    class Friend {
        std::unique_ptr<BaseFriend> Data;
        FriendType Type;

    public:
        Friend(FriendType Type, const Web::Epic::Responses::GetFriendsSummary::BaseUser& User) :
            Data(std::make_unique<FriendBaseUser>(User)),
            Type(Type)
        {

        }

        Friend(FriendType Type, const Web::Epic::Responses::GetFriendsSummary::SuggestedFriend& User) :
            Data(std::make_unique<FriendSuggested>(User)),
            Type(Type)
        {

        }

        Friend(FriendType Type, const Web::Epic::Responses::GetFriendsSummary::RealFriend& User) :
            Data(std::make_unique<FriendReal>(User)),
            Type(Type)
        {

        }

        Friend(FriendType Type) :
            Data(std::make_unique<FriendCurrent>()),
            Type(Type)
        {

        }

        FriendType GetType() const {
            return Type;
        }

        BaseFriend& Get() const {
            return *Data;
        }

        template<FriendType TargetType, std::enable_if_t<TargetType == FriendType::BLOCKED, bool> = true>
        FriendBaseUser& Get() const {
            return (FriendBaseUser&)Get();
        }

        template<FriendType TargetType, std::enable_if_t<TargetType == FriendType::SUGGESTED || TargetType == FriendType::OUTBOUND || TargetType == FriendType::INBOUND, bool> = true>
        FriendSuggested& Get() const {
            return (FriendSuggested&)Get();
        }

        template<FriendType TargetType, std::enable_if_t<TargetType == FriendType::NORMAL, bool> = true>
        FriendReal& Get() const {
            return (FriendReal&)Get();
        }

        template<FriendType TargetType, std::enable_if_t<TargetType == FriendType::CURRENT, bool> = true>
        FriendCurrent& Get() const {
            return (FriendCurrent&)Get();
        }

        std::weak_ordering operator<=>(const Friend& that) const {
            if (auto cmp = Type <=> that.Type; cmp != 0)
                return cmp;

            switch (Type)
            {
            case FriendType::NORMAL:
                return Get<FriendType::NORMAL>() <=> that.Get<FriendType::NORMAL>();
            case FriendType::INBOUND:
            case FriendType::OUTBOUND:
            case FriendType::SUGGESTED:
                return Get<FriendType::SUGGESTED>() <=> that.Get<FriendType::SUGGESTED>();
            case FriendType::BLOCKED:
                return Get<FriendType::BLOCKED>() <=> that.Get<FriendType::BLOCKED>();
            }

            return std::weak_ordering::equivalent;
        }
    };
}