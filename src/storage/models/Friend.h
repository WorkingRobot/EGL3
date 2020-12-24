#pragma once

#include "../../utils/Crc32.h"
#include "../../utils/StringCompare.h"
#include "../../web/epic/responses/Responses.h"
#include "../../web/xmpp/Responses.h"

#include <functional>
#include <set>
#include <shared_mutex>
#include <variant>
#include <vector>

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

    class BaseFriend {
    protected:
        using CallbackFunc = std::function<void(const BaseFriend&)>;

        std::optional<CallbackFunc> OnUpdate;

        std::string KairosAvatar, KairosBackground;

        BaseFriend() = default;

    public:
        BaseFriend(BaseFriend&&) = default;
        BaseFriend& operator=(BaseFriend&&) = default;

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

        void UpdateCallback() const {
            if (OnUpdate.has_value()) {
                OnUpdate.value()(*this);
            }
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

    public:
        FriendBaseUser(const Web::Epic::Responses::GetFriendsSummary::BaseUser& User) :
            AccountId(User.AccountId),
            Username(User.DisplayName.value_or(User.AccountId))
        {

        }

        FriendBaseUser(const std::string& AccountId, const std::string& Username) :
            AccountId(AccountId),
            Username(Username)
        {

        }

        const std::string& GetAccountId() const override {
            return AccountId;
        }

        const std::string& GetUsername() const override {
            return Username;
        }

        void SetUsername(const std::string& NewUsername) {
            Username = NewUsername;
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

    class FriendRequested : public FriendBaseUser {
    protected:
        // Connections aren't used (yet), so no reason to save them

    public:
        FriendRequested(const Web::Epic::Responses::GetFriendsSummary::RequestedFriend& User) :
            FriendBaseUser(User.AccountId, GetValidUsername(User))
        {

        }

        FriendRequested(FriendBaseUser&& other) : FriendBaseUser(std::move(other)) {

        }

        std::weak_ordering operator<=>(const FriendRequested& that) const {
            return FriendBaseUser::operator<=>(that);
        }

    private:
        std::string GetValidUsername(const Web::Epic::Responses::GetFriendsSummary::RequestedFriend& User) const {
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

        const std::string_view GetProductId() const {
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
            Ret.Status.SetProductName("EGL3");
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
            return Ret;
        }
    };

    enum class FriendType : uint8_t {
        BLOCKED,
        OUTBOUND,
        INBOUND,
        NORMAL,
        CURRENT,

        INVALID = 0xFF // If it doesn't belong anywhere, etc.
    };

    class Friend {
        enum DataType : uint8_t {
            UNKNOWN,
            BASE_USER, // Same as BLOCKED, but the DisplayName can be invalid/blank
            BLOCKED,
            REQUESTED,
            REAL,
            CURRENT
        };

        template<FriendType Type, DataType RealType, class T>
        struct ConstructType {
            explicit ConstructType() = default;
        };

        mutable std::shared_mutex Mutex;
        std::unique_ptr<BaseFriend> Data;
        DataType RealType;
        FriendType Type;

    public:
        Friend(Friend&&) = default;
        Friend& operator=(Friend&&) = default;

        static inline constexpr ConstructType<FriendType::INVALID, BASE_USER, FriendBaseUser> ConstructInvalid{};
        static inline constexpr ConstructType<FriendType::BLOCKED, BLOCKED, FriendBaseUser> ConstructBlocked{};
        static inline constexpr ConstructType<FriendType::OUTBOUND, REQUESTED, FriendRequested> ConstructOutbound{};
        static inline constexpr ConstructType<FriendType::INBOUND, REQUESTED, FriendRequested> ConstructInbound{};
        static inline constexpr ConstructType<FriendType::NORMAL, REAL, FriendReal> ConstructNormal{};
        static inline constexpr ConstructType<FriendType::CURRENT, CURRENT, FriendCurrent> ConstructCurrent{};

        template<FriendType Type, DataType RealType, class T, class... FriendArgs>
        Friend(ConstructType<Type, RealType, T> ConstructType, FriendArgs&&... Args) :
            Data(std::make_unique<T>(std::move(Args)...)),
            RealType(RealType),
            Type(Type)
        {

        }

        // Block user (and reject request or remove as friend)
        void SetBlocked(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            UpdateInternalData(FriendType::BLOCKED, Client, AsyncFF);
        }

        // Unblock user
        void SetUnblocked(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            if (GetType() == FriendType::BLOCKED) {
                UpdateInternalData(FriendType::INVALID, Client, AsyncFF);
            }
        }

        // Reject request or remove as friend
        void SetRemoveFriendship(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            auto Type = GetType();
            if (Type == FriendType::INBOUND || Type == FriendType::OUTBOUND || Type == FriendType::NORMAL) {
                UpdateInternalData(FriendType::INVALID, Client, AsyncFF);
            }
        }

        // Add user as a friend
        void SetAddFriendship(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            auto Type = GetType();
            if (Type == FriendType::INBOUND || Type == FriendType::OUTBOUND) {
                UpdateInternalData(FriendType::NORMAL, Client, AsyncFF);
            }
        }

        // User sent a friend request
        void SetRequestInbound(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            auto Type = GetType();
            if (Type == FriendType::INVALID || Type == FriendType::BLOCKED) {
                UpdateInternalData(FriendType::INBOUND, Client, AsyncFF);
            }
        }

        // User recieved a friend request
        void SetRequestOutbound(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            auto Type = GetType();
            if (Type == FriendType::INVALID || Type == FriendType::BLOCKED) {
                UpdateInternalData(FriendType::OUTBOUND, Client, AsyncFF);
            }
        }

        void InitializeAccountSettings(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            AsyncFF.Enqueue([&Client, this]() {
                std::unique_lock Lock(Mutex);

                auto AccResp = Client.GetSettingsForAccounts(std::vector<std::string>{ Data->GetAccountId() }, { "avatar", "avatarBackground" });
                EGL3_ASSERT(!AccResp.HasError(), "Kairos data request returned an error");

                for (auto& AccountSetting : AccResp->Values) {
                    if (AccountSetting.AccountId == Data->GetAccountId()) {
                        Data->UpdateAccountSetting(AccountSetting);
                    }
                }

                Lock.unlock();
                Get().UpdateCallback();
            });
        }

    private:
        // This will run asynchronously, but a mutex is used during the operation to prevent any CAS-like issues
        void UpdateInternalData(FriendType TargetType, Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
            switch (TargetType)
            {
            case FriendType::INVALID:
            {
                std::unique_lock Lock(Mutex);
                Type = TargetType;

                Lock.unlock();
                Get().UpdateCallback();
                break;
            }
            case FriendType::BLOCKED:
            {
                if (RealType < BLOCKED) {
                    AsyncFF.Enqueue([TargetType, &Client, this]() {
                        std::unique_lock Lock(Mutex);

                        Type = TargetType;
                        auto AccResp = Client.GetAccounts(std::vector<std::string>{ Data->GetAccountId() });
                        EGL3_ASSERT(!AccResp.HasError(), "Account request returned an error");

                        EGL3_ASSERT(AccResp->Accounts.size() == 1, "Account reponse does not have exactly 1 user");
                        GetUnlocked<FriendBaseUser>().SetUsername(AccResp->Accounts.front().GetDisplayName());
                        RealType = BLOCKED;

                        Lock.unlock();
                        Get().UpdateCallback();
                    });
                }
                else {
                    std::unique_lock Lock(Mutex);
                    Type = TargetType;

                    Lock.unlock();
                    Get().UpdateCallback();
                }
                break;
            }
            case FriendType::OUTBOUND:
            case FriendType::INBOUND:
            {
                if (RealType < BLOCKED) {
                    AsyncFF.Enqueue([TargetType, &Client, this]() {
                        std::unique_lock Lock(Mutex);

                        Type = TargetType;
                        Data.reset(new FriendRequested(std::move(GetUnlocked<FriendBaseUser>())));
                        RealType = REQUESTED;

                        auto AccResp = Client.GetAccounts(std::vector<std::string>{ Data->GetAccountId() });
                        EGL3_ASSERT(!AccResp.HasError(), "Account request returned an error");
                        EGL3_ASSERT(AccResp->Accounts.size() == 1, "Account reponse does not have exactly 1 user");
                        GetUnlocked<FriendRequested>().SetUsername(AccResp->Accounts.front().GetDisplayName());

                        Lock.unlock();
                        Get().UpdateCallback();
                    });
                }
                else if (RealType < REQUESTED) {
                    AsyncFF.Enqueue([TargetType, &Client, this]() {
                        std::unique_lock Lock(Mutex);

                        Type = TargetType;
                        Data.reset(new FriendRequested(std::move(GetUnlocked<FriendBaseUser>())));
                        RealType = REQUESTED;

                        Lock.unlock();
                        Get().UpdateCallback();
                    });
                }
                else {
                    std::unique_lock Lock(Mutex);
                    Type = TargetType;

                    Lock.unlock();
                    Get().UpdateCallback();
                }
                break;
            }
            case FriendType::NORMAL:
            {
                if (RealType < REAL) {
                    AsyncFF.Enqueue([TargetType, &Client, this]() {
                        std::unique_lock Lock(Mutex);

                        Type = TargetType;

                        auto AccResp = Client.GetFriend(Data->GetAccountId());
                        EGL3_ASSERT(!AccResp.HasError(), "Friend info request returned an error");

                        Data.reset(new FriendReal(AccResp.Get()));
                        RealType = REAL;

                        Lock.unlock();
                        Get().UpdateCallback();
                    });
                }
                else {
                    std::unique_lock Lock(Mutex);
                    Type = TargetType;

                    Lock.unlock();
                    Get().UpdateCallback();
                }
                break;
            }
            }
        }

        template<class T = BaseFriend, std::enable_if_t<std::is_base_of_v<BaseFriend, T>, bool> = true>
        T& GetUnlocked() const {
            return (T&)*Data;
        }

    public:
        FriendType GetType() const {
            std::shared_lock Lock(Mutex);
            return Type;
        }

        template<class T = BaseFriend, std::enable_if_t<std::is_base_of_v<BaseFriend, T>, bool> = true>
        T& Get() const {
            std::shared_lock Lock(Mutex);
            return GetUnlocked<T>();
        }

        std::weak_ordering operator<=>(const Friend& that) const {
            std::shared_lock Lock(Mutex);

            if (auto cmp = Type <=> that.Type; cmp != 0)
                return cmp;

            switch (Type)
            {
            case FriendType::NORMAL:
                return GetUnlocked<FriendReal>() <=> that.GetUnlocked<FriendReal>();
            case FriendType::INBOUND:
            case FriendType::OUTBOUND:
                return GetUnlocked<FriendRequested>() <=> that.GetUnlocked<FriendRequested>();
            case FriendType::BLOCKED:
            case FriendType::INVALID:
                return GetUnlocked<FriendBaseUser>() <=> that.GetUnlocked<FriendBaseUser>();
            }

            return std::weak_ordering::equivalent;
        }
    };
}