#pragma once

#include "../../web/epic/responses/GetSettingsForAccounts.h"

#include <functional>
#include <optional>

namespace EGL3::Storage::Models {
    class FriendBase {
    protected:
        using CallbackFunc = std::function<void(const FriendBase&)>;

        std::optional<CallbackFunc> OnUpdate;

        std::string KairosAvatar, KairosBackground;

        FriendBase() = default;

    public:
        FriendBase(FriendBase&&) = default;
        FriendBase& operator=(FriendBase&&) = default;

        virtual const std::string& GetAccountId() const = 0;

        virtual const std::string& GetUsername() const = 0;

        virtual const std::string& GetNickname() const = 0;

        virtual const std::string& GetKairosAvatar() const;

        virtual const std::string& GetKairosBackground() const;

        void SetKairosAvatar(const std::string& Avatar);

        void SetKairosBackground(const std::string& Background);

        const std::string GetKairosAvatarUrl() const;

        const std::string GetKairosBackgroundUrl() const;

        const std::string& GetDisplayName() const;

        const std::string& GetSecondaryName() const;

        template<typename... Args>
        void SetUpdateCallback(Args&&... Callback) {
            OnUpdate.emplace(std::forward<Args>(Callback)...);
        }

        void UpdateCallback() const;

        void UpdateAccountSetting(const Web::Epic::Responses::GetSettingsForAccounts::AccountSetting& FriendSetting);
    };
}