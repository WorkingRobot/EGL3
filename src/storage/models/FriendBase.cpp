#include "FriendBase.h"

#include "../../modules/Friends/KairosMenu.h"
#include "../../utils/Crc32.h"

namespace EGL3::Storage::Models {
    const std::string& FriendBase::GetKairosAvatar() const {
        return KairosAvatar;
    }

    const std::string& FriendBase::GetKairosBackground() const {
        return KairosBackground;
    }

    void FriendBase::SetKairosAvatar(const std::string& Avatar) {
        KairosAvatar = Avatar;

        OnUpdate.emit();
    }

    void FriendBase::SetKairosBackground(const std::string& Background) {
        KairosBackground = Background;

        OnUpdate.emit();
    }

    const std::string FriendBase::GetKairosAvatarUrl() const {
        return Modules::Friends::KairosMenuModule::GetKairosAvatarUrl(GetKairosAvatar());
    }

    const std::string FriendBase::GetKairosBackgroundUrl() const {
        return Modules::Friends::KairosMenuModule::GetKairosBackgroundUrl(GetKairosBackground());
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

    void FriendBase::UpdateAccountSetting(const Web::Epic::Responses::GetSettingsForAccounts::AccountSetting& FriendSetting) {
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

        OnUpdate.emit();
    }
}