#include "Friend.h"

#include "../../utils/Log.h"

namespace EGL3::Storage::Models {
    void Friend::QueueUpdate(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        if (EGL3_ENSURE(GetType() == FriendType::NORMAL, LogLevel::Warning, "Friend update is requested for a non-normal relationship")) {
            AsyncFF.Enqueue([&Client, this]() {
                std::unique_lock Lock(Mutex);

                auto AccResp = Client.GetFriend(Data->GetAccountId());
                if (!EGL3_ENSURE(!AccResp.HasError(), LogLevel::Error, "Friend info request returned an error")) {
                    return;
                }

                GetUnlocked<FriendReal>().UpdateInfo(AccResp.Get());

                Lock.unlock();
                Get().OnUpdate.emit();
            });
        }
    }

    void Friend::SetBlocked(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        UpdateInternalData(FriendType::BLOCKED, Client, AsyncFF);
    }

    void Friend::SetUnblocked(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        if (GetType() == FriendType::BLOCKED) {
            UpdateInternalData(FriendType::INVALID, Client, AsyncFF);
        }
    }

    void Friend::SetRemoveFriendship(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        auto Type = GetType();
        if (Type == FriendType::INBOUND || Type == FriendType::OUTBOUND || Type == FriendType::NORMAL) {
            UpdateInternalData(FriendType::INVALID, Client, AsyncFF);
        }
    }

    void Friend::SetAddFriendship(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        auto Type = GetType();
        if (Type == FriendType::INBOUND || Type == FriendType::OUTBOUND) {
            UpdateInternalData(FriendType::NORMAL, Client, AsyncFF);
        }
    }

    void Friend::SetRequestInbound(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        auto Type = GetType();
        if (Type == FriendType::INVALID || Type == FriendType::BLOCKED) {
            UpdateInternalData(FriendType::INBOUND, Client, AsyncFF);
        }
    }

    void Friend::SetRequestOutbound(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        auto Type = GetType();
        if (Type == FriendType::INVALID || Type == FriendType::BLOCKED) {
            UpdateInternalData(FriendType::OUTBOUND, Client, AsyncFF);
        }
    }

    void Friend::InitializeAccountSettings(Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        AsyncFF.Enqueue([&Client, this]() {
            std::unique_lock Lock(Mutex);

            auto AccResp = Client.GetSettingsForAccounts(std::vector<std::string>{ Data->GetAccountId() }, { "avatar", "avatarBackground" });
            if (!EGL3_ENSURE(!AccResp.HasError(), LogLevel::Error, "Kairos data request returned an error")) {
                return;
            }

            for (auto& AccountSetting : AccResp->Values) {
                if (AccountSetting.AccountId == Data->GetAccountId()) {
                    Data->UpdateAccountSetting(AccountSetting);
                }
            }

            Lock.unlock();
            Get().OnUpdate.emit();
        });
    }

    void Friend::UpdateInternalData(FriendType TargetType, Web::Epic::EpicClientAuthed& Client, Modules::AsyncFFModule& AsyncFF) {
        switch (TargetType)
        {
        case FriendType::INVALID:
        {
            std::unique_lock Lock(Mutex);
            Type = TargetType;

            Lock.unlock();
            Get().OnUpdate.emit();
            break;
        }
        case FriendType::BLOCKED:
        {
            if (RealType < BLOCKED) {
                AsyncFF.Enqueue([TargetType, &Client, this]() {
                    std::unique_lock Lock(Mutex);

                    Type = TargetType;
                    auto AccResp = Client.GetAccounts(std::vector<std::string>{ Data->GetAccountId() });
                    if (!EGL3_ENSURE(!AccResp.HasError(), LogLevel::Error, "Account request returned an error")) {
                        return;
                    }
                    if (!EGL3_ENSURE(AccResp->Accounts.size() == 1, LogLevel::Error, "Account reponse does not have exactly 1 user")) {
                        return;
                    }
                    GetUnlocked<FriendBaseUser>().SetUsername(AccResp->Accounts.front().GetDisplayName());
                    RealType = BLOCKED;

                    Lock.unlock();
                    Get().OnUpdate.emit();
                });
            }
            else {
                std::unique_lock Lock(Mutex);
                Type = TargetType;

                Lock.unlock();
                Get().OnUpdate.emit();
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
                    if (!EGL3_ENSURE(!AccResp.HasError(), LogLevel::Error, "Account request returned an error")) {
                        return;
                    }
                    if (!EGL3_ENSURE(AccResp->Accounts.size() == 1, LogLevel::Error, "Account reponse does not have exactly 1 user")) {
                        return;
                    }
                    GetUnlocked<FriendRequested>().SetUsername(AccResp->Accounts.front().GetDisplayName());

                    Lock.unlock();
                    Get().OnUpdate.emit();
                });
            }
            else if (RealType < REQUESTED) {
                AsyncFF.Enqueue([TargetType, &Client, this]() {
                    std::unique_lock Lock(Mutex);

                    Type = TargetType;
                    Data.reset(new FriendRequested(std::move(GetUnlocked<FriendBaseUser>())));
                    RealType = REQUESTED;

                    Lock.unlock();
                    Get().OnUpdate.emit();
                });
            }
            else {
                std::unique_lock Lock(Mutex);
                Type = TargetType;

                Lock.unlock();
                Get().OnUpdate.emit();
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
                    if (!EGL3_ENSURE(!AccResp.HasError(), LogLevel::Error, "Friend info request returned an error")) {
                        return;
                    }

                    Data.reset(new FriendReal(std::move(GetUnlocked<FriendBaseUser>())));
                    RealType = REAL;

                    GetUnlocked<FriendReal>().UpdateInfo(AccResp.Get());

                    Lock.unlock();
                    Get().OnUpdate.emit();
                });
            }
            else {
                std::unique_lock Lock(Mutex);
                Type = TargetType;

                Lock.unlock();
                Get().OnUpdate.emit();
            }
            break;
        }
        }
    }

    FriendType Friend::GetType() const {
        std::shared_lock Lock(Mutex);
        return Type;
    }

    std::weak_ordering Friend::operator<=>(const Friend& that) const
    {
        std::shared_lock ThisLock(Mutex, std::defer_lock);
        std::shared_lock ThatLock(that.Mutex, std::defer_lock);
        std::lock(ThisLock, ThatLock);

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
}