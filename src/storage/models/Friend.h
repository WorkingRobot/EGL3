#pragma once

#include "../../modules/AsyncFF.h"
#include "../../web/epic/EpicClientAuthed.h"
#include "FriendCurrent.h"

#include <shared_mutex>

namespace EGL3::Storage::Models {
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
        std::unique_ptr<FriendBase> Data;
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

        // Update nickname, etc.
        void QueueUpdate(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        // Block user (and reject request or remove as friend)
        void SetBlocked(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        // Unblock user
        void SetUnblocked(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        // Reject request or remove as friend
        void SetRemoveFriendship(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        // Add user as a friend
        void SetAddFriendship(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        // User sent a friend request
        void SetRequestInbound(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        // User recieved a friend request
        void SetRequestOutbound(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        void InitializeAccountSettings(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

    private:
        // This will run asynchronously, but a mutex is used during the operation to prevent any CAS-like issues
        void UpdateInternalData(FriendType TargetType, Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF);

        template<class T = FriendBase, std::enable_if_t<std::is_base_of_v<FriendBase, T>, bool> = true>
        T& GetUnlocked() const {
            return (T&)*Data;
        }

    public:
        FriendType GetType() const;

        template<class T = FriendBase, std::enable_if_t<std::is_base_of_v<FriendBase, T>, bool> = true>
        T& Get() const {
            std::shared_lock Lock(Mutex);
            return GetUnlocked<T>();
        }

        std::weak_ordering operator<=>(const Friend& that) const;
    };
}