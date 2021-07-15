#pragma once

#include "../../utils/Callback.h"
#include "../../web/epic/responses/GetSettingsForAccounts.h"

#include <sigc++/sigc++.h>

namespace EGL3::Storage::Models {
    class FriendBase {
    protected:
        std::string KairosAvatar, KairosBackground;

        FriendBase() = default;

    public:
        FriendBase(FriendBase&&) = default;
        FriendBase& operator=(FriendBase&&) = default;

        virtual ~FriendBase() = default;

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

        void UpdateAccountSetting(const Web::Epic::Responses::GetSettingsForAccounts::AccountSetting& FriendSetting);

        sigc::signal<void()> OnUpdate;
    };
}