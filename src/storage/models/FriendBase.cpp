#include "FriendBase.h"

#include "../../web/xmpp/PresenceKairosProfile.h"

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

    const std::string& FriendBase::GetKairosAvatar() const {
        return KairosAvatar;
    }

    const std::string& FriendBase::GetKairosBackground() const {
        return KairosBackground;
    }

    void FriendBase::SetKairosAvatar(const std::string& Avatar) {
        KairosAvatar = Avatar;
    }

    void FriendBase::SetKairosBackground(const std::string& Background) {
        KairosBackground = Background;
    }

    const std::string FriendBase::GetKairosAvatarUrl() const {
        return PresenceKairosProfile::GetKairosAvatarUrl(GetKairosAvatar());
    }

    const std::string FriendBase::GetKairosBackgroundUrl() const {
        return PresenceKairosProfile::GetKairosBackgroundUrl(GetKairosBackground());
    }

    const std::string& FriendBase::GetDisplayName() const {
        if (GetNickname().empty()) {
            return GetUsername();
        }
        return GetNickname();
    }

    const std::string& FriendBase::GetSecondaryName() const {
        if (GetNickname().empty()) {
            return GetNickname(); // Returns ""
        }
        return GetUsername();
    }

    void FriendBase::UpdateCallback() const {
        if (OnUpdate.has_value()) {
            OnUpdate.value()(*this);
        }
    }

    void FriendBase::UpdateAccountSetting(const Web::Epic::Responses::GetSettingsForAccounts::AccountSetting& FriendSetting) {
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
}